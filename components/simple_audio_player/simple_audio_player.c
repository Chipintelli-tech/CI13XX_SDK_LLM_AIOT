#include "user_config.h"
#if SIMPLE_AUDIO_PLAYER_ENABLE
#include <string.h>
#include "user_config.h"
#include "simple_audio_player.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "stream_buffer.h"
#include "romlib_runtime.h"
#include "codec_manager.h"
#include "ci_log_config.h"
#include "ci_log.h"
#include "board.h"
#include "flash_rw_process.h"
#include "player_data_fetcher.h"
#include "status_share.h"
#include "cias_aiot_protocol.h"
#include "cias_network_msg_protocol.h"

// #define SAP_MAX_PCM_FRAME_SIZE (2048)

#define SAP_MSG_QUEUE_LENGTH 10
#define SAP_PCM_BUF_COUNT 4
SemaphoreHandle_t fetch_sem =NULL;
extern CiasAiotFuncParamTypedef gCiasAiotFuncParam;
extern CiasAiotRunParamTypedef gCiasAiotRunParam;
#if SAP_FIX_DEVICE_POP_ENABLE
static int32_t last_gain = 0;
static int32_t smp_current_gain = 0;    //保存当前播放音量，用于播放开始和停止时的淡入、淡出
#endif
#if  AUDIO_PLAYER_ENABLE || NET_AUDIO_PLAY_BY_MP3
uint32_t total_bytes = 0;
int id3v2_size = 0;
uint32_t gap_index = 0;
#endif
typedef enum
{
    SAP_MSG_START_PLAY = 0, // Start playing. This message is always valid. If the sap is not in the SAP_STATE_IDLE state,the last play work will be stoped,and then begin a play work for this message.
    SAP_MSG_STOP_PLAY,      // Force stop play. This message is valid only when the sap is in the "SAP_STATE_START" or "SAP_STATE_PLAY" state.
} sap_msg_id_t;

typedef struct
{
    sap_msg_id_t msg_id;
    uint32_t data_addr;
    uint32_t data_size;
    uint32_t request_block_size;
    uint32_t fetcher_buffer_size;
    sap_src_type_t src_type;
    SAP_PLAY_END_CALLBACK play_end_callback;
    SAP_PLAY_DATA_REQUEST_CALLBACK data_request_callback;
    char *fmt;
} sap_msg_t;

typedef struct
{
    QueueHandle_t msg_queue;
    sap_status_t state;
    // uint32_t data_addr;
    SAP_PLAY_END_CALLBACK play_end_callback;
    void *decoder_handle;
    audio_decoder_interface_t *decoder_chain;
    audio_decoder_interface_t *cur_decoder;
    uint32_t sample_rate;
    uint32_t output_samples;
    uint32_t pcm_buffer_size;
    short *pcm_buffer;
    // uint32_t source_file_bytes_left;
    uint32_t source_buf_bytes_left;
    uint8_t decode_err_count;
    uint32_t source_buf_offset;
    uint32_t decode_frame_count;
    // uint32_t source_frame_size;
    uint32_t frame_pcm_size;
    // uint32_t total_pcm_size;
    uint8_t source_data_buf[SAP_SOURCE_DATA_BUF_SIZE];
    uint8_t channels;
    #if NET_AUDIO_PLAY_BY_MP3
    int32_t mp3_one_frame_size;
    #endif
} sap_info_t;

sap_info_t sap_info = {
    .msg_queue = NULL,
    .state = SAP_STATE_IDLE,
    .pcm_buffer = NULL,
    // .source_file_bytes_left = 0,
    .source_buf_bytes_left = 0,
    .decoder_chain = NULL,
};

sap_src_type_t src_type; // 数据源类型
static audio_play_state_t audio_play_state = AUDIO_PLAY_STATE_IDLE;
static void task_simple_audio_player(void *pvParameters);
static void sap_send_msg(sap_msg_t *msg, BaseType_t *xHigherPriorityTaskWoken);

/**
 * @brief Initialize the simple audio player module.
 *
 * @return int 1: successed; not 1: failed.
 */
int sap_init(void)
{
    return (int)xTaskCreate(task_simple_audio_player, "simple-audio-player", 512, 0, 4, NULL);
}

/**
 * @brief Start playing.
 *
 * @param data_addr uint32_t It's used to specifiy the address of the audio data.
 * @param play_end_callback A pointer to a function that will be called when play to end.
 * @return int 1: successed; not 1: failed.
t */
int sap_play(uint32_t data_addr, void *play_end_callback)
{
    gCiasAiotRunParam.request_play_data_flag = false;
    #if NET_AUDIO_PLAY_BY_PCM || NET_AUDIO_PLAY_BY_G722 || NET_AUDIO_PLAY_BY_MP3
    gCiasAiotRunParam.cloud_play_state = CLOUD_PLAY_END;
    #endif
    sap_msg_t msg =
        {
            .msg_id = SAP_MSG_START_PLAY,
            .data_addr = data_addr,
            .data_size = 0,
            .fetcher_buffer_size = SAP_SOURCE_DATA_BUF_SIZE,
            .src_type = SAP_DATA_SRC_FLASH,
            .play_end_callback = play_end_callback,
            .fmt = "MP3",
        };
    sap_send_msg(&msg, NULL);
}

int sap_play_stream(const char *audio_fmt, uint32_t fetcher_buffer_size, uint32_t request_block_size, void *play_end_callback, SAP_PLAY_DATA_REQUEST_CALLBACK data_request_callback)
{
    sap_msg_t msg =
        {
            .msg_id = SAP_MSG_START_PLAY,
            .data_addr = 0,
            .data_size = 0,
            .request_block_size = request_block_size,
            .fetcher_buffer_size = fetcher_buffer_size,
            .src_type = SAP_DATA_SRC_STREAM,
            .play_end_callback = play_end_callback,
            .fmt = audio_fmt,
            .data_request_callback = (SAP_PLAY_DATA_REQUEST_CALLBACK)data_request_callback,
        };
    sap_send_msg(&msg, NULL);
}

sap_status_t sap_get_state(void)
{
    return sap_info.state;
}

// 停止接口
void sap_stop()
{
    if (sap_info.state == SAP_STATE_IDLE)
    {
        return;
    }
    #if NET_AUDIO_PLAY_BY_PCM || NET_AUDIO_PLAY_BY_G722 || NET_AUDIO_PLAY_BY_MP3
    gCiasAiotRunParam.cloud_play_state = CLOUD_PLAY_END;
    #endif
    sap_msg_t msg;
    msg.msg_id = SAP_MSG_STOP_PLAY;
    uint8_t timeout_cnt = 0;
    while (sap_info.decode_frame_count < 8 && sap_info.state != SAP_STATE_IDLE && timeout_cnt<100)
    {
        timeout_cnt++;
        vTaskDelay(1);
    }
    sap_send_msg(&msg, NULL);
    vTaskDelay(2);
}

/**
 * @brief 调节播放音量
 *
 * @param gain 音量(0--100)
 */
void audio_play_set_vol_gain(int32_t gain)
{
    //ci_loginfo(LOG_USER,"\n gain=%02X\n",gain)
    #if SAP_FIX_DEVICE_POP_ENABLE
    smp_current_gain = gain;
    if(last_gain != gain)
    {
        cm_set_codec_dac_gain(PLAY_CODEC_ID, 0, gain);
        last_gain = gain;
    }
    #else
    cm_set_codec_dac_gain(PLAY_CODEC_ID, 0, gain);
    #endif
}

/**
 * @brief pa、da控制，可选择是否控制功放
 *
 * @param cmd pa使能或失能
 *
 */
void audio_play_hw_pa_da_ctl(FunctionalState cmd, bool is_control_pa)
{
    // PA的操作需要在DAC关闭的情况下进行，不然会影响采音
    if (ENABLE == cmd)
    {
        #if SAP_FIX_DEVICE_POP_ENABLE
        //打开DAC/功放之前，先把DAC增益压到0（静音），避免DAC使能、功放开启时的电平跳变被放大成pop音；
        //等真正的音频数据从0幅值淡入后再恢复音量（在smp_decode_one_frame中恢复）
        cm_set_codec_dac_gain(PLAY_CODEC_ID, 0, 0);
        last_gain = 0;
        #endif
        icodec_start(CODEC_OUTPUT);
        if (is_control_pa)
        {
            power_amplifier_on();
        }
    }
    else
    {
        if (is_control_pa)
        {
            power_amplifier_off();
        }
        icodec_stop(CODEC_OUTPUT);
    }
}

static void sap_send_msg(sap_msg_t *msg, BaseType_t *xHigherPriorityTaskWoken)
{
    if (0 != check_curr_trap())
    {
        xQueueSendFromISR(sap_info.msg_queue, msg, xHigherPriorityTaskWoken);
    }
    else
    {
        xQueueSend(sap_info.msg_queue, msg, portMAX_DELAY);
    }
}

/**
 * @brief Inner start playing function.
 *
 * @param msg A pointer to a sap_msg_t structure used to input data address and callback function.
 * @return int 0:successed, not 0:failed.
 */
static int sap_start_play_inner(sap_msg_t *msg)
{
    int ret = 0;
    // uint8_t *tmp_buf;
    do
    {
        // 搜索解码器
        sap_info.cur_decoder = NULL;
        for (audio_decoder_interface_t *decoder = sap_info.decoder_chain; decoder; decoder = decoder->next)
        {
            if (0 == strcmp(decoder->name, msg->fmt))
            {
                sap_info.cur_decoder = decoder;
                break;
            }
        }
        if (NULL == sap_info.cur_decoder)
        {
            ci_logerr(CI_LOG_ERROR, "no decoder found\n");
            ret = -1;
            break;
        }
        // sap_info.data_addr = msg->data_addr;
        sap_info.play_end_callback = msg->play_end_callback;
        sap_info.decoder_handle = sap_info.cur_decoder->init();

        src_data_info_t src_data_info = {
            .src_type = msg->src_type,
            .data_addr = msg->data_addr,
            .data_size = msg->data_size,
            .data_offset = 0,
            .buffer_size = msg->fetcher_buffer_size,
            .request_block_size = msg->request_block_size,
            .data_request_callback = msg->data_request_callback,
        };
        src_type = msg->src_type;
        if(pdf_init(&src_data_info))
        {
            ret = -1;
            break;
        }

        // malloc a buffer to recevie output data from decoder.
        // tmp_buf = pvPortMalloc(SAP_MAX_PCM_FRAME_SIZE);
        // if (!tmp_buf)
        // {
        //     ci_logerr(CI_LOG_ERROR, "not enough memory\n");
        //     ret = -1;
        //     break;
        // }
        int32_t read_size = pdf_fetch_data(sap_info.source_data_buf, SAP_SOURCE_DATA_BUF_SIZE, pdMS_TO_TICKS(1000));
        ci_logdebug(LOG_AUDIO_PLAY, "sap read %d bytes\n", read_size);
        if (read_size <= 0)
        {
            ci_logerr(CI_LOG_ERROR, "read data failed\n");
            ret = -2;
            break;
        }

        uint32_t pcm_data_size;
        audio_format_info_t format_info = {0};
        int bytes_left = read_size;
        #if NET_AUDIO_PLAY_BY_MP3
        int get_info_rst = 0;
        // int temp = bytes_left;
        while (1)
        {
            get_info_rst = sap_info.cur_decoder->get_info(sap_info.decoder_handle, sap_info.source_data_buf, &bytes_left, NULL, &format_info);
            if (get_info_rst == -1)
            {
                mprintf("ID3 gaping \r\n");
                read_size = pdf_fetch_data(sap_info.source_data_buf, SAP_SOURCE_DATA_BUF_SIZE, pdMS_TO_TICKS(200));
                bytes_left = read_size;
                continue;
            }
            else if (get_info_rst < -1)
            {
                ci_logerr(CI_LOG_ERROR, "get format info failed\n");
                ret = -3;
                break;
            }
            else
            {
                break;
            }
        }

        // bytes_left = temp;
        #else
        if (sap_info.cur_decoder->get_info(sap_info.decoder_handle, sap_info.source_data_buf, &bytes_left, NULL, &format_info) < 0)
        {
            ci_logerr(CI_LOG_ERROR, "get format info failed\n");
            ret = -3;
            break;
        }
        #endif
        #if  AUDIO_PLAYER_ENABLE || NET_AUDIO_PLAY_BY_MP3
        total_bytes = 0;
        id3v2_size = 0;
        gap_index = 0;
        #endif
#if (PLAYER_CONTROL_PA)
        if(!gCiasAiotRunParam.pa_start_flag)
        {
            audio_play_hw_pa_da_ctl(ENABLE, true);
            gCiasAiotRunParam.pa_start_flag =true;
            mprintf("pa_on\r\n");
            vTaskDelay(pdMS_TO_TICKS(100));
        }
#else
        audio_play_hw_pa_da_ctl(ENABLE,false);
#endif

        if (format_info.src_data_size > 0 && src_type == SAP_DATA_SRC_FLASH)
        {
            pdf_set_total_data_size(format_info.src_data_size);
        }
        // sap_info.total_pcm_size = format_info.pcm_data_size;
        sap_info.source_buf_bytes_left = bytes_left;
        sap_info.source_buf_offset = read_size - bytes_left;
        sap_info.output_samples = format_info.samples_per_frame;
        sap_info.decode_frame_count = 0;
        #if NET_AUDIO_PLAY_BY_MP3
        sap_info.mp3_one_frame_size = 0;
        #endif
        sap_info.frame_pcm_size = sap_info.output_samples * sizeof(short);
        // if (sap_info.frame_pcm_size > SAP_MAX_PCM_FRAME_SIZE)
        // {
        //     ci_logerr(LOG_AUDIO_PLAY, "pcm frame size %d too large!\n", sap_info.frame_pcm_size);
        // }

        cm_pcm_buffer_info_t pcm_buffer_info;
        pcm_buffer_info.play_buffer_info.block_num = 2;
        pcm_buffer_info.play_buffer_info.buffer_num = SAP_PCM_BUF_COUNT;
        pcm_buffer_info.play_buffer_info.buffer_size = sap_info.frame_pcm_size;
        pcm_buffer_info.play_buffer_info.block_size = pcm_buffer_info.play_buffer_info.buffer_size / pcm_buffer_info.play_buffer_info.block_num;
        int pcm_buffer_total_size = pcm_buffer_info.play_buffer_info.buffer_size * pcm_buffer_info.play_buffer_info.buffer_num;
        if (sap_info.pcm_buffer_size < pcm_buffer_total_size)
        {
            if (sap_info.pcm_buffer != NULL)
            {
                vPortFree(sap_info.pcm_buffer);
                sap_info.pcm_buffer = NULL;
            }
        }

        if (sap_info.pcm_buffer == NULL)
        {
            sap_info.pcm_buffer = pvPortMalloc(pcm_buffer_total_size);
            // mprintf("pvPortMalloc %d\r\n",pcm_buffer_total_size);
            if (!sap_info.pcm_buffer)
            {
                mprintf("not enough memory pcm_buffer\n");
                ret = -5;
                break;
            }
            sap_info.pcm_buffer_size = pcm_buffer_total_size;
        }
        pcm_buffer_info.play_buffer_info.pcm_buffer = sap_info.pcm_buffer;
        cm_config_pcm_buffer(PLAY_CODEC_ID, CODEC_OUTPUT, &pcm_buffer_info);
        // uint32_t ret_buf;
        // cm_get_pcm_buffer(PLAY_CODEC_ID,&ret_buf,portMAX_DELAY);
        // memcpy((void*)ret_buf, tmp_buf, mp3FrameInfo.outputSamps*sizeof(short)*mp3FrameInfo.nChans);
        // cm_write_codec(PLAY_CODEC_ID, (void*)ret_buf,portMAX_DELAY);
        // static uint32_t pre_sample_rate = 0;
        // static uint32_t pre_nChans = 0;
        // if((sap_info.sample_rate != format_info.samprate) || (sap_info.channels != format_info.channels))
        {
            sap_info.sample_rate = format_info.samprate;
            sap_info.channels = format_info.channels;

            cm_sound_info_t sound_info;
            sound_info.sample_rate = format_info.samprate;
            sound_info.sample_depth = IIS_DW_16BIT;
            sound_info.channel_flag = (format_info.channels == 2) ? 3 : 1;
            cm_config_codec(PLAY_CODEC_ID, CODEC_OUTPUT, &sound_info);
        }
    } while (0);

    // if (tmp_buf)
    // {
    //     vPortFree(tmp_buf);
    // }
    mprintf("Play start\r\n");
    return ret;
}

static void wait_codec_play_finish()
{
    while (cm_get_codec_empty_buffer_number(PLAY_CODEC_ID, CODEC_OUTPUT) < SAP_PCM_BUF_COUNT - 1)
    {
        vTaskDelay(1);
    }
}
#if SAP_FIX_DEVICE_POP_ENABLE
/**
 * @brief 对送入硬件播放的第一帧PCM数据做线性淡入，避免音频从非0幅值突然开始产生pop音
 * 
 * @param pcm_buf 一帧PCM数据的首地址
 */
static void smp_fade_in_first_frame(void *pcm_buf)
{
    if (0 == sap_info.sample_rate || 0 == sap_info.channels || 0 == sap_info.output_samples)
    {
        return;
    }

    uint32_t ramp_points = sap_info.sample_rate / 200;      //5ms的淡入时间
    uint32_t total_points = sap_info.output_samples / sap_info.channels;
    if (ramp_points > total_points)
    {
        ramp_points = total_points;
    }

    int16_t *pcm = (int16_t *)pcm_buf;
    for (uint32_t i = 0; i < ramp_points; i++)
    {
        for (uint32_t ch = 0; ch < sap_info.channels; ch++)
        {
            pcm[i * sap_info.channels + ch] =
                (int16_t)((int32_t)pcm[i * sap_info.channels + ch] * (int32_t)i / (int32_t)ramp_points);
        }
    }
}

/**
 * @brief 停止/打断播放前，把DAC增益分档平滑降到0（淡出），避免音频信号被瞬间切断产生pop音
 * 
 */
static void smp_fade_out_and_mute(void)
{
    if (smp_current_gain > 0)
    {
        int32_t step = (smp_current_gain + 15) / 16;    //约16步降完，总时长十几ms
        if (step < 1)
        {
            step = 1;
        }
        for (int32_t gain = smp_current_gain; gain > 0; gain -= step)
        {
            cm_set_codec_dac_gain(PLAY_CODEC_ID, 0, gain);
            vTaskDelay(1);
        }
    }
    cm_set_codec_dac_gain(PLAY_CODEC_ID, 0, 0);
    last_gain = 0;
}
#endif
/**
 * @brief Decode one frame.
 *
 * @return int  0: Decode failed;
 *              1: Decode successed,and the PCM buffer of codec manager is full, you can do some delay before the next call to this function to free up CPU for other tasks;
 *              2: Decode successed,and the PCM buffer of codec manager is not full, you'd better call this function again immediately.
 *              3: End of file, there's no source data any more, end of play.
 */
static int sap_decode_one_frame(void)
{
    int ret = 0;
    uint32_t pcm_buf;
    int bytes_left = sap_info.source_buf_bytes_left;
    int32_t read_size;
#if MP3_NO_CONSTANT_BITRATE
    extern int get_cur_frame_size( uint8_t *data, int *bytes_left);
#endif
    while (1)
    {
        //ci_logwarn(LOG_AUDIO_PLAY, "sap source_buf_bytes_left %d\n", sap_info.source_buf_bytes_left);
        read_size = SAP_SOURCE_DATA_BUF_SIZE - sap_info.source_buf_bytes_left;
        if (read_size > 0)
        {
            if(sap_info.source_buf_offset !=0)
            memmove(sap_info.source_data_buf, &sap_info.source_data_buf[sap_info.source_buf_offset], bytes_left);
            sap_info.source_buf_offset = 0;
            // read_size = pdf_fetch_data(&sap_info.source_data_buf[sap_info.source_buf_bytes_left], read_size, pdMS_TO_TICKS(200));
            read_size = pdf_fetch_data(&sap_info.source_data_buf[sap_info.source_buf_bytes_left], read_size, pdMS_TO_TICKS(32));     
            // if (wait_flag)
            // {
            //     /*等待云端数据*/
            //     vTaskDelay(50);
            //     wait_flag = false;
            // }
           // ci_logwarn(LOG_AUDIO_PLAY, "sap read %d\n", read_size);

#if MP3_NO_CONSTANT_BITRATE
            if(src_type == SAP_DATA_SRC_STREAM)//本地为恒定比特率不需要计算
            {
                sap_info.mp3_one_frame_size = get_cur_frame_size(&sap_info.source_data_buf[sap_info.source_buf_offset],&bytes_left);
                if(sap_info.mp3_one_frame_size == -1 && gCiasAiotRunParam.cloud_play_state == CLOUD_DATA_PLAYING)
                {
                    mprintf("erro mp3 frame size,no find sync\r\n");
                    continue;
                }
                mprintf("sap_info.mp3_one_frame_size = %d\r\n",sap_info.mp3_one_frame_size);
            }
#endif
            if (read_size > 0 )
            {
                #if NET_AUDIO_PLAY_BY_MP3
                /*云端MP3等待数据达到一帧后才解码*/
                if((src_type == SAP_DATA_SRC_FLASH) || (src_type == SAP_DATA_SRC_STREAM &&(read_size + sap_info.source_buf_bytes_left>= sap_info.mp3_one_frame_size)))
	            {
                    bytes_left += read_size;
                    sap_info.source_buf_bytes_left = bytes_left;
                    break;
	            }
                else
                {
                    bytes_left += read_size;
                    sap_info.source_buf_bytes_left = bytes_left;
                    continue;
                }
                #else
                bytes_left += read_size;
                sap_info.source_buf_bytes_left = bytes_left;
                break;
                #endif
            }
            else if (read_size == -1 && sap_info.source_buf_bytes_left == 0) // 缓存内无数据，并且请求数据任务删除
            {
                wait_codec_play_finish();
                ret = 3;
                return ret;
            }
            else if (sap_info.source_buf_bytes_left > 0)
            {
#if NET_AUDIO_PLAY_BY_PCM
                /*pcm格式缓存内数据小于PCM_FRAME_SIZE需等待后续数据，否则播放任务会结束*/
#if AUDIO_PLAYER_ENABLE
                if (src_type == SAP_DATA_SRC_STREAM)
#endif   
                {
                    if(sap_info.source_buf_bytes_left >= NET_AUDIO_PCM_FRAME_SIZE || gCiasAiotRunParam.cloud_play_state != CLOUD_DATA_PLAYING)
                    break;
                }
#if AUDIO_PLAYER_ENABLE
                else
                {
                    /*PCM开启本地播报*/
                    break;
                }
#endif  
#elif  NET_AUDIO_PLAY_BY_G722
                if(gCiasAiotRunParam.cloud_play_state != CLOUD_DATA_PLAYING)
                {
                    break;
                }
#elif NET_AUDIO_PLAY_BY_MP3
                /*云端播报mp3格式时缓存内有数据，需等待云端后续的数据，否则解码寻找帧头会错*/
                if ((gCiasAiotRunParam.cloud_play_state != CLOUD_DATA_PLAYING) || (sap_info.source_buf_bytes_left >= sap_info.mp3_one_frame_size))
                {
                    // 云端播放超时结束,清除MP3缓存剩余数据
                    if (src_type == SAP_DATA_SRC_STREAM && gCiasAiotRunParam.cloud_play_state == CLOUD_PLAYING_DATA_WITING_TIMEOUT)
                    {
                        memset(sap_info.source_data_buf, 0, SAP_SOURCE_DATA_BUF_SIZE);
                    }
                    break;
                }
#else
                break;//无云端播报直接解码
#endif
                // wait_flag = true;
            }
            else
            {
                #if NET_AUDIO_PLAY_BY_PCM || NET_AUDIO_PLAY_BY_G722 || NET_AUDIO_PLAY_BY_MP3
                #if NET_AUDIO_PLAY_BY_MP3
                if((cm_get_codec_empty_buffer_number(PLAY_CODEC_ID, CODEC_OUTPUT) >= SAP_PCM_BUF_COUNT - 1) && gCiasAiotRunParam.play_cloud_end_flag)
                {
                    break;
                }
                #endif
                /*仅云端播放时,缓存没有数据,也没收到云端数据，需等待*/
                if (gCiasAiotRunParam.cloud_play_state != CLOUD_DATA_PLAYING)
                #endif
                {
                    break;
                }
                // wait_flag = true;
            }
        }
        else // 缓存满了，继续执行去解码
        {
            break;
        }
    }

    #if SAP_FIX_DEVICE_POP_ENABLE
    if(sap_info.decode_frame_count > SAP_DISCARD_FRAME_NUM)
    {
        cm_get_pcm_buffer(PLAY_CODEC_ID, &pcm_buf, portMAX_DELAY);
    }
    else
    {
        pcm_buf = (uint32_t)((void*)pvPortMalloc(sap_info.frame_pcm_size));
        if(pcm_buf == NULL)
        {
            mprintf("not enough memory\n");
        }
    }
    #else
        cm_get_pcm_buffer(PLAY_CODEC_ID, &pcm_buf, portMAX_DELAY);
    #endif
    int32_t err = sap_info.cur_decoder->decode_one_frame(sap_info.decoder_handle, &sap_info.source_data_buf[sap_info.source_buf_offset], &bytes_left, (void *)pcm_buf, 0); // Decode a frame.
   // ci_logwarn(LOG_AUDIO_PLAY, "audio decorde err %d,bytes_left %d\n", err, bytes_left);
    #if NET_AUDIO_PLAY_BY_MP3
    #if !MP3_NO_CONSTANT_BITRATE
    //恒定比特率直接赋值解码的大小
    if(sap_info.mp3_one_frame_size ==0 && src_type == SAP_DATA_SRC_STREAM)
    {
        sap_info.mp3_one_frame_size = sap_info.source_buf_bytes_left - bytes_left;
        mprintf("CBR mp3_one_frame_size =%d\r\n",sap_info.mp3_one_frame_size);
    }
    #endif
    #endif
    if (0 == err)
    {
        #if SAP_FIX_DEVICE_POP_ENABLE
        if(sap_info.decode_frame_count > SAP_DISCARD_FRAME_NUM)
        {
            if((SAP_DISCARD_FRAME_NUM + 1) == sap_info.decode_frame_count)
            {
                mprintf("smp_fade_in_first_frame\r\n");
                //送入硬件播放的第一帧数据做淡入，避免音频起始的pop音
                smp_fade_in_first_frame((void*)pcm_buf);
            }
        #endif
            if (bytes_left >= 0)
            {
                cm_write_codec(PLAY_CODEC_ID, (void *)pcm_buf, portMAX_DELAY);
            }
            else
            {
                cm_release_pcm_buffer(PLAY_CODEC_ID, CODEC_OUTPUT, (void *)pcm_buf);
                bytes_left = 0;
            }
        #if SAP_FIX_DEVICE_POP_ENABLE
        }
        else
        {
            //播放开头的预热帧直接丢弃，不送DAC播放
            vPortFree((void*)pcm_buf);
        }
        #endif

        int bytes_used = sap_info.source_buf_bytes_left - bytes_left;
        sap_info.source_buf_bytes_left = bytes_left;
        sap_info.source_buf_offset += bytes_used;
        sap_info.decode_frame_count += 1;
        sap_info.decode_err_count = 0;
        #if SAP_FIX_DEVICE_POP_ENABLE
        if (sap_info.decode_frame_count == SAP_DISCARD_FRAME_NUM+2)
        #else
        if (sap_info.decode_frame_count == 3)
        #endif
        {
            // mprintf("\r\n-----start_codec\r\n");
            cm_start_codec(PLAY_CODEC_ID, CODEC_OUTPUT);
            cm_set_codec_mute(PLAY_CODEC_ID, CODEC_OUTPUT, 3, DISABLE);
            #if SAP_FIX_DEVICE_POP_ENABLE
            //此时音频数据已从0幅值淡入，恢复DAC增益不会产生阶跃声
            cm_set_codec_dac_gain(PLAY_CODEC_ID, 0, smp_current_gain);
            last_gain = smp_current_gain;
            #endif
        }
        ret = 1;
    }
    else
    {
        // mprintf("audio decorde err %d,bytes_left %d\n", err, bytes_left);
        cm_release_pcm_buffer(PLAY_CODEC_ID, CODEC_OUTPUT, (void *)pcm_buf);

        #if NET_AUDIO_PLAY_BY_MP3
        //MP3帧错误,都先跳解码消耗的字节，进行重同步, 不要直接结束播放
        if(gCiasAiotRunParam.cloud_play_state == CLOUD_DATA_PLAYING)
        {
            ret = 1;
            if (bytes_left < 0 ) 
            {
                bytes_left = 0;
            }
            int bytes_used = sap_info.source_buf_bytes_left - bytes_left; 
            sap_info.source_buf_offset += bytes_used;
            sap_info.source_buf_bytes_left = bytes_left; 

            if (++sap_info.decode_err_count > MP3_MAX_DECODE_ERR)
            { 
                #if SAP_FIX_DEVICE_POP_ENABLE
                if(sap_info.decode_frame_count >= SAP_DISCARD_FRAME_NUM+2)
                #else
                if(sap_info.decode_frame_count >= 3)
                #endif
                {
                    wait_codec_play_finish();
                }
                ret = 0;//错帧超过SAP_MAX_DECODE_ERR次停止播放器
            } 
            // 立即重试，让 MP3FindSyncWord 找下一个真同步字
        }
        else
        #endif
        {
            #if SAP_FIX_DEVICE_POP_ENABLE
            if(sap_info.decode_frame_count >= SAP_DISCARD_FRAME_NUM+2)
            #else
            if(sap_info.decode_frame_count >= 3)
            #endif
            {
                wait_codec_play_finish();
            }
        }

        //ci_logwarn(LOG_AUDIO_PLAY, "audio decode err %d,bad frame!\n", err);
    }
    return ret;
}

static int sap_stop_play_inner(int32_t play_cb_state)
{
    int ret = 0;
    #if SAP_FIX_DEVICE_POP_ENABLE
    //停止/打断播放前先把音量平滑淡出，再静音和关闭codec，避免信号被瞬间切断产生pop音
    smp_fade_out_and_mute();
    #endif
    cm_set_codec_mute(PLAY_CODEC_ID, CODEC_OUTPUT, 3, ENABLE);
    cm_stop_codec(PLAY_CODEC_ID, CODEC_OUTPUT);
    sap_info.cur_decoder->deinit(sap_info.decoder_handle);
    sap_info.decoder_handle = NULL;
    if (sap_info.pcm_buffer != NULL)
    {
        // mprintf("vPortFree\r\n");
        vPortFree(sap_info.pcm_buffer);
        sap_info.pcm_buffer = NULL;
    }
    pdf_deinit();
    #if NET_AUDIO_PLAY_BY_MP3
    sap_info.mp3_one_frame_size = 0;
    #endif
    if(src_type == SAP_DATA_SRC_FLASH)
    {
        cias_send_cmd(LOCAL_AUDIO_PLAY_STOP, DEF_FILL);
    }

    if(CI_SS_PLAY_STATE_PLAYING == ciss_get(CI_SS_PLAY_STATE))
    {
#if USE_AEC_MODULE
        if(src_type == SAP_DATA_SRC_FLASH && !check_combination_index_state())
        {
            ciss_set(CI_SS_PLAY_STATE, CI_SS_PLAY_STATE_PLAYING_TO_IDLE); // 设置播放结束
        }
        else if(src_type == SAP_DATA_SRC_STREAM)
        {
            ciss_set(CI_SS_PLAY_STATE, CI_SS_PLAY_STATE_PLAYING_TO_IDLE); // 设置播放结束
        }
        else
        {
            /*noting*/
        }
#else
        ciss_set(CI_SS_PLAY_STATE, CI_SS_PLAY_STATE_IDLE); // 设置播放结束
#endif
    }
    mprintf("Play end state =%d\r\n", play_cb_state);
    if (sap_info.play_end_callback)
    {
        sap_info.play_end_callback(play_cb_state);
    }
    return ret;
}

/**
 * @brief Task function of player
 *
 * @param pvParameters Task parameter.
 */
static void task_simple_audio_player(void *pvParameters)
{
    sap_info.state = SAP_STATE_IDLE; // 播放状态初始化
    audio_play_state = AUDIO_PLAY_STATE_IDLE;
    TickType_t wait_time = portMAX_DELAY;
    fetch_sem = xSemaphoreCreateMutex();
    if (NULL == fetch_sem)
    {
        mprintf("fetch_MuxSem creat fail\r\n");
        return;
    }
    sap_info.msg_queue = xQueueCreate(SAP_MSG_QUEUE_LENGTH, sizeof(sap_msg_t));
    while (1)
    {
        sap_msg_t msg;
        BaseType_t rst = xQueueReceive(sap_info.msg_queue, &msg, wait_time);
        if (rst == pdTRUE) // If received a message from the message queue.
        {
            // Received a message.
            if (sap_info.state != SAP_STATE_IDLE)
            {
                // stop last play.
                sap_info.state = SAP_STATE_STOPPING;
                sap_stop_play_inner(-1);
                // mprintf("SAP_STATE_IDLE2\r\n");
                sap_info.state = SAP_STATE_IDLE;
                audio_play_state = AUDIO_PLAY_STATE_IDLE;
            }
            wait_time = 0;
            if (msg.msg_id == SAP_MSG_START_PLAY)
            {
                // start current play request.
                sap_info.state = SAP_STATE_STARTING;
                audio_play_state = AUDIO_PLAY_STATE_START;
                int rst = sap_start_play_inner(&msg);
                if (rst != 0)
                {
                    // ci_logerr(LOG_AUDIO_PLAY, "start play failed,err %d!\n", rst);
                    sap_info.state = SAP_STATE_STOPPING;
                    sap_stop_play_inner(-1);
                    sap_info.state = SAP_STATE_IDLE;
                    audio_play_state = AUDIO_PLAY_STATE_IDLE;
                    wait_time = portMAX_DELAY;
                }
                else
                {
                    ciss_set(CI_SS_PLAY_STATE, CI_SS_PLAY_STATE_PLAYING);
                    sap_info.state = SAP_STATE_PLAYING;
                    if (src_type == SAP_DATA_SRC_STREAM)
                    {
                        #if NET_AUDIO_PLAY_BY_PCM || NET_AUDIO_PLAY_BY_G722 || NET_AUDIO_PLAY_BY_MP3
                        gCiasAiotRunParam.cloud_play_state = CLOUD_DATA_PLAYING;
                        #endif
                    }
                    else if(src_type == SAP_DATA_SRC_FLASH)
                    {
                        cias_send_cmd(LOCAL_AUDIO_PLAY_START, DEF_FILL);
                    }
                    else
                    {
                        /*nothing*/
                    }
                    audio_play_state = AUDIO_PLAY_STATE_PLAYING;
                }
            }
        }
        else
        {
            // No message
            if (sap_info.state == SAP_STATE_PLAYING)
            {
                // decode one frame
                int rst = sap_decode_one_frame();
                if (rst == 3 || rst == 0)
                {
                    ci_logdebug(LOG_AUDIO_PLAY, "end err %d\n", rst);
                    sap_info.state = SAP_STATE_STOPPING;
                    sap_stop_play_inner(0);
                    // mprintf("SAP_STATE_IDLE3\r\n");
                    sap_info.state = SAP_STATE_IDLE;
                    audio_play_state = AUDIO_PLAY_STATE_IDLE;
                    wait_time = portMAX_DELAY;
                }
                else
                {
                    wait_time = rst ? 0 : 2;
                }
            }
            else if (sap_info.state == SAP_STATE_IDLE)
            {
                wait_time = portMAX_DELAY;
            }
            else
            {
                wait_time = 0;
            }
        }
    }
}

int sap_register_decoder(audio_decoder_interface_t *decoder)
{
    if (decoder == NULL)
    {
        return -1;
    }
    decoder->next = sap_info.decoder_chain;
    sap_info.decoder_chain = decoder;
    return 0;
}

/**
 * @brief 返回播放器task状态
 *
 * @return uint32_t 播放处理任务状态-兼容自学习中播报状态
 */
uint32_t get_audio_play_state(void)
{
    return audio_play_state;
}
#endif
