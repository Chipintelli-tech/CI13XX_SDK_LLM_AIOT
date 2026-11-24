#include "codec_manager.h"
#include "ci130x_audio_pre_rslt_out.h"
#include <string.h>
#include "ci130x_codec.h"
#include "ci130x_dpmu.h"
#include "board.h"
#include "ci130x_gpio.h"
#include <stdbool.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "sdk_default_config.h"
#include "ci130x_uart.h"
#include "ci130x_dma.h"
#include "debug_time_consuming.h"
#include "stream_buffer.h"
#include "cias_voice_upload.h"
#include "cias_aiot_protocol.h"
#include "status_share.h"
#include "user_config.h"
#include "ci_audio_wrapfft.h"
#include "aec_gain_tools.h"
#include "uart_dump_tools.h"
#if USE_NN_DENOISE && DENOISE_STRENGTH_ADAPT_ENABLE
#include "noise_energy_estimation_api.h"
#endif
//开启IIS0输出，并且采样率不是16hz的时候需要使用升降采样
#if UP_DOWN_ENABLE
#include "ci_alg_common_api.h"
ci_common_handle_t * ci_common_handle= NULL;
int16_t *g_frame_tmp = NULL;
#endif
#define BUFFER_NUM (4)
typedef struct
{
    audio_pre_rslt_out_init_t init_str;
    // 写了多少次数据
    uint32_t write_data_cnt;
    // 已经发送了多少次数据，写数据频次是固定的，我们不能改变，只能改变发送数据的频度，
    // 如果发送太快，会在mute模式再发送上一个buf，
    // 如果发送太慢，会扔掉一个buf不发送
    uint32_t send_data_cnt;

    int32_t write_send_sub_slave; // 在slave模式下，write和send的差值
    bool codec_start_flag[MAX_CODEC_NUM];        //codec启动标志，需缓存9帧否则会出现buffer无数据填0的情况
    // 是否使用硬件tx merge功能
    bool hardware_tx_merge;
} audio_pre_init_tmp_t;

static audio_pre_init_tmp_t sg_init_tmp_str;

#define PI (3.1416926f)
void sine_wave_generate(int16_t *sine_wave, uint32_t sample_rate, uint32_t wave_fre, uint32_t point_num)
{
    int16_t num_of_one_period = sample_rate / wave_fre; // 每个周期的采样点数
    for (int i = 0; i < point_num; i++)
    {
        sine_wave[i] = (int16_t)(32767.0f * sinf((2 * PI * i) / num_of_one_period));
    }
}

void audio_pre_rslt_out_play_card_init(void)
{
    uint16_t block_size = AUDIO_CAP_POINT_NUM_PER_FRM * 2 * sizeof(int16_t);
#if (UP_DOWN_ENABLE)
#if (UP_DOWN_FLAG == 1)
    uint32_t flag = CI_COMMON_UPSAMPLE;
#else
    uint32_t flag = CI_COMMON_DOWNSAMPLE;
#endif
    ci_common_handle = ci_common_create(flag,0,NULL);
    if(ci_common_handle == NULL)
    mprintf("ci_common_create fail\r\n");
    g_frame_tmp = (int16_t *)pvPortMalloc(AUDIO_CAP_POINT_NUM_PER_FRM * 2 * sizeof(int16_t));
#endif
    sg_init_tmp_str.init_str.block_size = block_size;
    sg_init_tmp_str.write_data_cnt = 0;
    sg_init_tmp_str.send_data_cnt = 0;
    memset(sg_init_tmp_str.codec_start_flag,false,MAX_CODEC_NUM);
#if DEBUG_AUDIO_UART_UPLOAD_EN
    uart_send_voice_init();
#endif

#if AUDIO_DATA_PLAY_BY_IIS || USE_HP_OUT_PRE_RSLT_AUDIO ||(!SIMPLE_AUDIO_PLAYER_ENABLE && NET_AUDIO_PLAY_BY_PCM)
    audio_pre_rslt_out_codec_init_pa_out();
#endif

#if USE_IIS1_OUT_PRE_RSLT_AUDIO || AUDIO_DATA_UPLOAD_BY_IIS
    audio_pre_rslt_out_codec_init();
#endif

}
void codec_output_ctl(int codec_index)
{
    if (MAX_CODEC_NUM <= codec_index)
    {
        mprintf("codec_index err\r\n");
        return;
    }
    int cnt = cm_get_codec_busy_buffer_number(codec_index,CODEC_OUTPUT);
    if (!sg_init_tmp_str.codec_start_flag[codec_index])
    {
        /*busy buffer 大于10帧再开启codec，否则会出现数据量不够的情况*/
        if(cnt > 10)
        {
            mprintf("--%d codec start--\r\n",codec_index);
            cm_start_codec(codec_index, CODEC_OUTPUT);
            sg_init_tmp_str.codec_start_flag[codec_index] = true;
        }
    }
    else
    {
        if(cnt == 0)
        {
            mprintf("--%d codec stop--\r\n",codec_index);
            cm_stop_codec(codec_index, CODEC_OUTPUT);
            sg_init_tmp_str.codec_start_flag[codec_index] = false;
        }
    }
}

extern CiasAiotRunParamTypedef gCiasAiotRunParam;
extern CiasAiotFuncParamTypedef gCiasAiotFuncParam;
#if IIS_CHANNEL_ENG_CALC_EANBLE
/*计算能量*/
void alg_calc_audio_eng(short *p_audio_pcm_data, uint16_t audio_fft_data_len, uint32_t *p_dst_db, uint32_t *P_dst_eng)
{
    float eng_tmp_avg = 0.0f, eng_tmp = 0.0f, dst_db_cur = 0.0f;
    for(int i = 0; i < audio_fft_data_len; i++)
    {
        eng_tmp += p_audio_pcm_data[i]*p_audio_pcm_data[i];
    }
    eng_tmp_avg = sqrtf(eng_tmp/audio_fft_data_len/32767.0f);
    *p_dst_db = (uint32_t)(20*log10f(eng_tmp_avg/6.0f) + 102.0f);
    *P_dst_eng = sqrt(eng_tmp/audio_fft_data_len);
}

static void audio_eng_calc(ci_wrapfft_audio * p_wrapfft_audio)
{
    static int calc_eng_frame_interval = 0; 
    if(calc_eng_frame_interval++ == ENG_CALC_INTERVAL_FRAME)
    {
        calc_eng_frame_interval = 0;
        //计算左mic能量和DB
        if(gCiasAiotFuncParam.micl_eng_db_calc_flag)
        {
            alg_calc_audio_eng(p_wrapfft_audio->mic[0], AUDIO_CAP_POINT_NUM_PER_FRM, &gCiasAiotFuncParam.micl_db, &gCiasAiotFuncParam.micl_eng);
            //mprintf("micl dst_db = %d\r\n", gCiasAiotFuncParam.micl_db);
           // mprintf("micl dst_eng = %d\r\n", gCiasAiotFuncParam.micl_eng);
        }
        //计算右mic能量和DB
        if(gCiasAiotFuncParam.micr_eng_db_calc_flag)
        {
            alg_calc_audio_eng(p_wrapfft_audio->mic[1], AUDIO_CAP_POINT_NUM_PER_FRM, &gCiasAiotFuncParam.micr_db, &gCiasAiotFuncParam.micr_eng);
           // mprintf("micr dst_db = %d\r\n", gCiasAiotFuncParam.micr_db);
           // mprintf("micr dst_eng = %d\r\n", gCiasAiotFuncParam.micr_eng);
        }
        //计算参考信号左通道能量和DB
        if(gCiasAiotFuncParam.refl_eng_db_calc_flag)
        {
            alg_calc_audio_eng(p_wrapfft_audio->ref[0], AUDIO_CAP_POINT_NUM_PER_FRM, &gCiasAiotFuncParam.refl_db, &gCiasAiotFuncParam.refl_eng);
           // mprintf("refl dst_db = %d\r\n", gCiasAiotFuncParam.refl_db);
           // mprintf("refl dst_eng = %d\r\n", gCiasAiotFuncParam.refl_eng);
        }
        //计算参考信号右通道能量和DB
        if(gCiasAiotFuncParam.refr_eng_db_calc_flag)
        {
            alg_calc_audio_eng(p_wrapfft_audio->ref[1], AUDIO_CAP_POINT_NUM_PER_FRM, &gCiasAiotFuncParam.refr_db, &gCiasAiotFuncParam.refr_eng);
          //  mprintf("refr dst_db = %d\r\n", gCiasAiotFuncParam.refr_db);
          //  mprintf("refr dst_eng = %d\r\n", gCiasAiotFuncParam.refr_eng);
        }
    }   
}
#endif
#if USE_AUDIO_UPLOAD_BY_IIS

void audio_pre_rslt_upload_by_iis(int16_t *left, int16_t *right, ci_wrapfft_audio *p_wrapfft_audio)
{
    uint32_t write_pcm_addr = 0;
    uint32_t block_size = sg_init_tmp_str.init_str.block_size;
    int num = block_size / sizeof(int16_t) / 2;
#if IIS_UPLOAD_IS_WAKEUP
    if ((gCiasAiotRunParam.is_wake_up_flag || gCiasAiotRunParam.is_always_iis_flag))  
#endif 
    {
        cm_get_pcm_buffer(PLAY_PRE_AUDIO_CODEC_ID, &write_pcm_addr, 0); // TODO HSL
        if (0 == write_pcm_addr)
        {
            mprintf("write_pcm_addr buffer is overflow\r\n");
            return;
        }
        int16_t *pcm_data_p = (int16_t *)write_pcm_addr;
        #if (UP_DOWN_ENABLE)
        int out_num = num;
        #if (UP_DOWN_FLAG == 1)
        out_num *= IIS0_BUF_NUM_RATIO;
        #elif (UP_DOWN_FLAG == 2)
        out_num /= IIS0_BUF_NUM_RATIO;
        #endif
        #endif
        for (int i = 0; i < num; i++)
        {     
            if (gCiasAiotFuncParam.upload_audio_by_denoise) // 上传降噪音频，目前AEC时不进行降噪
            {
                #if (UP_DOWN_ENABLE)
                g_frame_tmp[2 * i] = left[i];   
                g_frame_tmp[2 * i + 1] = right[i];
                #else
                pcm_data_p[2 * i] = left[i];   
                pcm_data_p[2 * i + 1] = right[i];
                #endif
            }
            else
            {
                if(ciss_get(CI_SS_AEC_WORK_STATE)) 
                {
                    #if (UP_DOWN_ENABLE)
                    g_frame_tmp[2 * i] = left[i];   
                    g_frame_tmp[2 * i + 1] = right[i];
                    #else
                    pcm_data_p[2 * i] = left[i];   
                    pcm_data_p[2 * i + 1] = right[i];
                    #endif
                }
                else
                {
                    #if (UP_DOWN_ENABLE)
                    g_frame_tmp[2 * i] = p_wrapfft_audio->asr_src_data[i]; //降噪进行时使用降噪前的数据
                    g_frame_tmp[2 * i + 1] = p_wrapfft_audio->asr_src_data[i];
                    #else
                    pcm_data_p[2 * i] = p_wrapfft_audio->asr_src_data[i];   //降噪进行时使用降噪前的数据
                    pcm_data_p[2 * i + 1] = p_wrapfft_audio->asr_src_data[i];
                    #endif
                }
            }
        }
        #if (UP_DOWN_FLAG == 1)
        // 升采样左右声道
        upsample_linear_interp(ci_common_handle,g_frame_tmp,num,pcm_data_p,out_num,true);
        #elif (UP_DOWN_FLAG == 2)
        // 降采样左右声道
        downsample_process(ci_common_handle,g_frame_tmp,num,pcm_data_p,out_num,true);
        #endif
        cm_write_codec(PLAY_PRE_AUDIO_CODEC_ID, (void *)write_pcm_addr, 0);
    }
    codec_output_ctl(PLAY_PRE_AUDIO_CODEC_ID);
    sg_init_tmp_str.send_data_cnt++;
    sg_init_tmp_str.write_data_cnt++;
}
#elif USE_AUDIO_UPLOAD_BY_HPOUT
void audio_pre_rslt_upload_by_hpout(int16_t *left, int16_t *right, ci_wrapfft_audio *p_wrapfft_audio)
{
    int ret = 0;
    uint32_t block_size = sg_init_tmp_str.init_str.block_size;
    int num = block_size / sizeof(int16_t) / 2;
#if HPOUT_UPLOAD_IS_WAKEUP
    if ((gCiasAiotRunParam.is_wake_up_flag || gCiasAiotRunParam.is_always_hpout_flag)) 
#endif   
    {
        uint32_t write_pcm_addr_cpy = 0;
        cm_get_pcm_buffer(PLAY_CODEC_ID, &write_pcm_addr_cpy, 10); // TODO HSL
        if (0 == write_pcm_addr_cpy)
        {
            mprintf("write_pcm_addr_cpy buffer is overflow\r\n");
            return;
        }
        int16_t *pcm_data_p_cpy = (int16_t *)write_pcm_addr_cpy;
        for (int i = 0; i < num; i++)
        {
            if (gCiasAiotFuncParam.upload_audio_by_denoise) // 上传降噪音频，目前AEC时不进行降噪
            {
                pcm_data_p_cpy[2 * i] = right[i];   
                pcm_data_p_cpy[2 * i + 1] = right[i];
            }
            else
            {
                if(ciss_get(CI_SS_AEC_WORK_STATE)) 
                {
                    pcm_data_p_cpy[2 * i] = right[i];   
                    pcm_data_p_cpy[2 * i + 1] = right[i];
                }
                else
                {
                    pcm_data_p_cpy[2 * i] = p_wrapfft_audio->asr_src_data[i];   //降噪进行时使用降噪前的数据
                    pcm_data_p_cpy[2 * i + 1] = p_wrapfft_audio->asr_src_data[i];
                }
            }
        }
        cm_write_codec(PLAY_CODEC_ID, (void *)write_pcm_addr_cpy, 0);
    }
    codec_output_ctl(PLAY_CODEC_ID);
    
#if USE_IIS1_OUT_PRE_RSLT_AUDIO
    #if (UP_DOWN_ENABLE)
    int out_num = num;
    #if (UP_DOWN_FLAG == 1)
    out_num *= IIS0_BUF_NUM_RATIO;
    #elif (UP_DOWN_FLAG == 2)
    out_num /= IIS0_BUF_NUM_RATIO;
    #endif
    #endif
    uint32_t write_pcm_addr = 0;
    cm_get_pcm_buffer(PLAY_PRE_AUDIO_CODEC_ID, &write_pcm_addr, 0); // TODO HSL
    int16_t *pcm_data_p = (int16_t *)write_pcm_addr;
    for (int i = 0; i < num; i++)
    {
        #if (UP_DOWN_ENABLE)
        g_frame_tmp[2 * i] = left[i];   
        g_frame_tmp[2 * i + 1] = right[i];
        #else
        pcm_data_p[2 * i] = left[i];
        pcm_data_p[2 * i + 1] = right[i];
        #endif
    }
    if (0 == write_pcm_addr)
    {
        return;
    }
    #if (UP_DOWN_FLAG == 1)
    // 升采样左右声道
    upsample_linear_interp(ci_common_handle,g_frame_tmp,num,pcm_data_p,out_num,true);
    #elif (UP_DOWN_FLAG == 2)
    // 降采样左右声道
    downsample_process(ci_common_handle,g_frame_tmp,num,pcm_data_p,out_num,true);
    #endif
    cm_write_codec(PLAY_PRE_AUDIO_CODEC_ID, (void *)write_pcm_addr, 0);
    codec_output_ctl(PLAY_PRE_AUDIO_CODEC_ID);
#endif

    sg_init_tmp_str.send_data_cnt++;
    sg_init_tmp_str.write_data_cnt++;
}
#elif USE_AUDIO_UPLOAD_BY_UART
void audio_pre_rslt_upload_by_uart(int16_t *left, int16_t *right, ci_wrapfft_audio *p_wrapfft_audio)
{
    int ret = 0;
    uint32_t block_size = sg_init_tmp_str.init_str.block_size;
    int num = block_size / sizeof(int16_t) / 2;
    extern int8_t speex_src_temp_buf[PCM_ALG_FRAME_LEN];  //p_wrapfft_audio->asr_src_data);
#if AUDIO_DATA_UPLOAD_BY_UART
    if (!gCiasAiotFuncParam.upload_play_full_duplex)
    {
        if (CI_SS_PLAY_STATE_PLAYING == ciss_get(CI_SS_PLAY_STATE))
        {
            return;
        }
    }
#if UPLOAD_PCM_DATA_ENABLE
    if (1)
#else
    // mprintf("gCiasAiotRunParam.is_wake_up_flag = %d\r\n", gCiasAiotRunParam.is_wake_up_flag);
    // mprintf("gCiasAiotRunParam.stop_collect_pcm_flag = %d\r\n", gCiasAiotRunParam.stop_collect_pcm_flag);
    if (gCiasAiotRunParam.is_wake_up_flag && !gCiasAiotRunParam.stop_collect_pcm_flag)
#endif
    {
        if (gCiasAiotRunParam.is_vad_on_flag)
        {
            gCiasAiotRunParam.vad_start_pcm_frame_count++;
        }
        if (xStreamBufferIsFull(gCiasAiotRunParam.pcm_compress_stream_buffer)) // 缓存满开始清老的数据
        {
            // mprintf("wakeup_pcm_frame_len is full, remove pcm data: %d frame\r\n", PCM_MSG_STREAM_NUM - NN_VAD_FRONT_PORCH);

            for (int i = 0; i < (PCM_MSG_STREAM_NUM - NN_VAD_FRONT_PORCH); i++)
            {
                int rx_size = xStreamBufferReceiveFromISR(gCiasAiotRunParam.pcm_compress_stream_buffer, speex_src_temp_buf, PCM_ALG_FRAME_LEN, pdMS_TO_TICKS(5));
                if (rx_size != PCM_ALG_FRAME_LEN)
                {
                    mprintf("roll back pcm_compress_stream_buf rcv error1\r\n");
                }
                else
                {
                    gCiasAiotRunParam.wake_up_pcm_frame_count--;
                }
            }
        }
        gCiasAiotRunParam.wake_up_pcm_frame_count++;
        if (gCiasAiotRunParam.pcm_compress_stream_buffer)
        {
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            uint8_t *p_upload_src = NULL;

            if (gCiasAiotFuncParam.upload_audio_by_denoise) // 上传降噪音频，目前AEC时不进行降噪
            {
                ret = xStreamBufferSendFromISR(gCiasAiotRunParam.pcm_compress_stream_buffer, (int8_t *)right, PCM_ALG_FRAME_LEN, &xHigherPriorityTaskWoken);
            }
            else
            {
                if(ciss_get(CI_SS_AEC_WORK_STATE)) 
                {
                    p_upload_src = right;
                }
                else
                {
                    p_upload_src = p_wrapfft_audio->asr_src_data;   //降噪进行时使用降噪前的数据
                }
                //上传降噪之前，AEC处理后的数据
                ret = xStreamBufferSendFromISR(gCiasAiotRunParam.pcm_compress_stream_buffer, (int8_t *)p_upload_src, PCM_ALG_FRAME_LEN, &xHigherPriorityTaskWoken);
            }
 
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
            if (ret != PCM_ALG_FRAME_LEN)
            {
                mprintf("xSpeexRecordStreamBuffer send error, send len = %d\r\n", ret);
            }
        }
    }
#endif
#if USE_IIS1_OUT_PRE_RSLT_AUDIO || USE_HP_OUT_PRE_RSLT_AUDIO
#if USE_IIS1_OUT_PRE_RSLT_AUDIO
    #if (UP_DOWN_ENABLE)
    int out_num = num;
    #if (UP_DOWN_FLAG == 1)
    out_num *= IIS0_BUF_NUM_RATIO;
    #elif (UP_DOWN_FLAG == 2)
    out_num /= IIS0_BUF_NUM_RATIO;
    #endif
    #endif
    uint32_t write_pcm_addr = 0;
    cm_get_pcm_buffer(PLAY_PRE_AUDIO_CODEC_ID, &write_pcm_addr, 0); // TODO HSL
    int16_t *pcm_data_p = (int16_t *)write_pcm_addr;
    for (int i = 0; i < num; i++)
    {
        #if (UP_DOWN_ENABLE)
        g_frame_tmp[2 * i] = left[i];   
        g_frame_tmp[2 * i + 1] = right[i];
        #else
        pcm_data_p[2 * i] = left[i];
        pcm_data_p[2 * i + 1] = right[i];
        #endif
    }
    if (0 == write_pcm_addr)
    {
        return;
    }
    #if (UP_DOWN_FLAG == 1)
    // 升采样左右声道
    upsample_linear_interp(ci_common_handle,g_frame_tmp,num,pcm_data_p,out_num,true);
    #elif (UP_DOWN_FLAG == 2)
    // 降采样左右声道
    downsample_process(ci_common_handle,g_frame_tmp,num,pcm_data_p,out_num,true);
    #endif
    cm_write_codec(PLAY_PRE_AUDIO_CODEC_ID, (void *)write_pcm_addr, 0);
#endif

#if USE_HP_OUT_PRE_RSLT_AUDIO
    uint32_t write_pcm_addr_cpy = 0;
    cm_get_pcm_buffer(PLAY_CODEC_ID, &write_pcm_addr_cpy, 0); // TODO HSL
    int16_t *pcm_data_p_cpy = (int16_t *)write_pcm_addr_cpy;
    for (int i = 0; i < num; i++)
    {
        pcm_data_p_cpy[2 * i] = left[i];
        pcm_data_p_cpy[2 * i + 1] = right[i];
    }
    if (0 == write_pcm_addr_cpy)
    {
        return;
    }
    cm_write_codec(PLAY_CODEC_ID, (void *)write_pcm_addr_cpy, 0);
#endif
    #if USE_IIS1_OUT_PRE_RSLT_AUDIO
    codec_output_ctl(PLAY_PRE_AUDIO_CODEC_ID);
    #endif
    #if USE_HP_OUT_PRE_RSLT_AUDIO && !NET_AUDIO_PLAY_BY_PCM
    codec_output_ctl(PLAY_CODEC_ID);
    #endif
#endif
    sg_init_tmp_str.send_data_cnt++;
    sg_init_tmp_str.write_data_cnt++;
}

#else

#endif
/**
 * @brief 写数据到发送端
 *
 * @param rslt 处理结果的起始指针
 * @param origin 原始数据的起始指针
 *  此函数不可用于中断
 */
void audio_pre_rslt_write_data(int16_t *left, int16_t *right, uint32_t wrapfft_audio_addr)
{
    ci_wrapfft_audio *p_wrapfft_audio = (ci_wrapfft_audio *)wrapfft_audio_addr;   //p_wrapfft_audio->asr_src_data);

    #if USE_NN_DENOISE && DENOISE_STRENGTH_ADAPT_ENABLE
    extern noise_est_config_t nn_noise_est_config;
    if(nn_noise_est_config.noise_est_debug)
    {
        mprintf("noise_log_energy =%d\r\n",(int32_t)p_wrapfft_audio->noise_log_energy);
    }
    #endif
#if AUTO_GET_AEC_GAIN
    auto_get_aec_gain(p_wrapfft_audio);
#endif
#if DEBUG_AUDIO_UART_UPLOAD_EN
#if DEBUG_AUDIO_4CH_UART_UPLOAD_EN
    audio_debug_uart_4ch_upload(p_wrapfft_audio);
#else
    audio_debug_uart0_upload(left, right, p_wrapfft_audio);
#endif
#endif
#if IIS_CHANNEL_ENG_CALC_EANBLE
    audio_eng_calc(p_wrapfft_audio);
#endif
#if USE_AUDIO_UPLOAD_BY_IIS
    audio_pre_rslt_upload_by_iis(left,right,p_wrapfft_audio);
#elif USE_AUDIO_UPLOAD_BY_HPOUT
    audio_pre_rslt_upload_by_hpout(left,right,p_wrapfft_audio);
#elif USE_AUDIO_UPLOAD_BY_UART
    audio_pre_rslt_upload_by_uart(left,right,p_wrapfft_audio);
#else
#endif
}

/**
 * @brief 语音前处理输出停止
 *
 */
void audio_pre_rslt_stop(void)
{
#if USE_IIS1_OUT_PRE_RSLT_AUDIO || USE_HP_OUT_PRE_RSLT_AUDIO
#else

#if USE_IIS1_OUT_PRE_RSLT_AUDIO
    cm_stop_codec(PLAY_PRE_AUDIO_CODEC_ID, CODEC_OUTPUT);
#endif

#if USE_HP_OUT_PRE_RSLT_AUDIO
    cm_stop_codec(PLAY_CODEC_ID, CODEC_OUTPUT);
#endif
#endif

}

/**
 * @brief 语音前处理输出开始
 *
 */
void audio_pre_rslt_start(void)
{
#if USE_IIS1_OUT_PRE_RSLT_AUDIO || USE_HP_OUT_PRE_RSLT_AUDIO

#else

#if USE_IIS1_OUT_PRE_RSLT_AUDIO
    cm_start_codec(PLAY_PRE_AUDIO_CODEC_ID, CODEC_OUTPUT);
#endif
#if USE_HP_OUT_PRE_RSLT_AUDIO
    cm_start_codec(PLAY_CODEC_ID, CODEC_OUTPUT);
#endif
#endif

}

/********** (C) COPYRIGHT Chipintelli Technology Co., Ltd. *****END OF FILE****/
