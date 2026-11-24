#include "codec_manager.h"
#include <string.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "task.h"
#include "sdk_default_config.h"
#include "cias_aiot_protocol.h"
#include "status_share.h"
#include "user_config.h"
#include "aec_gain_tools.h"
#if AUTO_GET_AEC_GAIN && USE_AEC_MODULE
#include "alg_preprocess.h"
#include "cias_network_msg_protocol.h"
#include "es7243e.h"
#include "ci_adapt_aec.h"
extern aec_config_t aec_config;
extern float es7243e_init_gain;
#endif

extern CiasAiotRunParamTypedef gCiasAiotRunParam;
extern CiasAiotFuncParamTypedef gCiasAiotFuncParam;
/*****************************************AEC自动获取增益工具*******************************************/
#if AUTO_GET_AEC_GAIN && USE_AEC_MODULE
extern cm_codec_hw_info_t host_mic_hw_info;
extern audio_capture_t audio_capture;
extern void es7243e_alc_maxgain_set(float gain);
extern int8_t es7243e_alc_gate_en(es7243e_alc_gate_t ALC_gate);
void auto_get_aec_gain(ci_wrapfft_audio * p_wrapfft_audio)
{
    int64_t eng_mic_eng= 0;
    int64_t eng_ref_eng= 0;
    static uint8_t volume = 0;
    uint16_t real_len;
    uint8_t test_rslt = 0;
    static int32_t sum_mic_eng =0;
    static int32_t sum_ref_eng =0;
    static uint8_t cnt =0;
    static uint8_t err_cnt =0;
    static uint8_t frame_cnt =0;
    static float  aec_enable_thr = 0;
    static bool calc_flag = false;
 
    do
    {
        if(gCiasAiotFuncParam.aec_gain_get_state == GET_AEC_GAIN_MIC)
        {
            if(volume == 0)
            volume = vol_get();
            if (CI_SS_PLAY_STATE_PLAYING == ciss_get(CI_SS_PLAY_STATE))
            {
                calc_flag = true;
                if(cnt >= 60 && frame_cnt<150)
                {
                    for(int i = 0; i < AUDIO_CAP_POINT_NUM_PER_FRM; i++)
                    {
                        eng_mic_eng += p_wrapfft_audio->mic[0][i] * p_wrapfft_audio->mic[0][i];
                    }
                    sum_mic_eng += sqrt(eng_mic_eng/AUDIO_CAP_POINT_NUM_PER_FRM); 
                    frame_cnt++;
                }
                else
                cnt++;
            }
            else if(CI_SS_PLAY_STATE_IDLE == ciss_get(CI_SS_PLAY_STATE) && calc_flag)
            {
                if(sum_mic_eng != 0)
                {
                    if(sum_mic_eng/frame_cnt > ENERGE_UP_THR)
                    {
                        mprintf("cur eng = %d\r\n",sum_mic_eng/frame_cnt);
                        if(aec_config.alc_off_codec_adc_gain_l >= (gCiasAiotFuncParam.aec_min_mic_gain+2))
                        {
                            mprintf("now_pga_gain_mic =%d\r\n",aec_config.alc_off_codec_adc_gain_l);
                            #if !REF_IN_FROM_INNER_CODEC
                            if(audio_capture.mic_channel_num == 2)
                            {
                                aec_config.alc_off_codec_adc_gain_l -= 2;
                                aec_config.alc_off_codec_adc_gain_r -= 2;
                            }
                            else
                            {
                                aec_config.alc_off_codec_adc_gain_l -= 2;
                            }
                            #else
                            //TODO
                            #endif
                        }
                        else if(volume > VOLUME_MIN)
                        {
                            volume -= 1;
                            mprintf("now_volume =%d\r\n",volume);
                            audio_play_set_vol_gain(67*volume/VOLUME_MAX + 7);
                        }
                        else
                        {
                            test_rslt = 0;
                            cias_send_cmd_and_data(CIAS_AEC_GAIN_RESULT, &test_rslt, 1, DEF_FILL);
                        }                   
                    }
                    else
                    {
                        mprintf("dst eng = %d\r\n",sum_mic_eng/frame_cnt);
                        gCiasAiotFuncParam.aec_gain_get_state = GET_AEC_GAIN_REF;
                        err_cnt = 0;
                        mprintf("\r\n-----------------GET_AEC_GAIN_REF START-----------------\r\n");
                    }
                    sum_mic_eng =0;
                    frame_cnt =0;
                    cnt = 0;
                    prompt_play_by_voice_id(GET_AEC_GAIN_AUDIO_ID, NULL, true);  //播放测试音频
                }
                else
                {
                    err_cnt++;
                    break;
                }
                calc_flag = false;
            }  
        }
        if(gCiasAiotFuncParam.aec_gain_get_state == GET_AEC_GAIN_REF)
        {
            if (CI_SS_PLAY_STATE_PLAYING == ciss_get(CI_SS_PLAY_STATE))
            {
                calc_flag = true;
                if(cnt >= 60 && frame_cnt<150)
                {
                    for(int i = 0; i < AUDIO_CAP_POINT_NUM_PER_FRM; i++)
                    {
                        eng_mic_eng += p_wrapfft_audio->mic[0][i] * p_wrapfft_audio->mic[0][i];
                        eng_ref_eng += p_wrapfft_audio->ref[0][i] * p_wrapfft_audio->ref[0][i];
                    }
                    sum_mic_eng += sqrt(eng_mic_eng/AUDIO_CAP_POINT_NUM_PER_FRM); 
                    sum_ref_eng += sqrt(eng_ref_eng/AUDIO_CAP_POINT_NUM_PER_FRM);
                    frame_cnt++;
                }
                else
                cnt++;
            }
            else if(CI_SS_PLAY_STATE_IDLE == ciss_get(CI_SS_PLAY_STATE) && calc_flag)
            {
                if(sum_mic_eng != 0 && sum_ref_eng != 0)
                {
                    // mprintf("sum_ref_eng =%d\r\n",sum_ref_eng/frame_cnt);
                    // mprintf("sum_mic_eng =%d\r\n",sum_mic_eng/frame_cnt);
                    if((float)sum_mic_eng/sum_ref_eng > 6.0f)
                    {
                        #if !REF_IN_FROM_INNER_CODEC
                        if(audio_capture.mic_channel_num == 2)
                        {
                            es7243e_init_gain += 2.0f;
                            mprintf("now_pga_gain_ref =%d\r\n",(int32_t)es7243e_init_gain);
                            es7243e_alc_gate_en(ES7243E_ALC_ON);
                            es7243e_alc_maxgain_set(es7243e_init_gain);
                            es7243e_alc_gate_en(ES7243E_ALC_OFF);
                        }
                        else
                        #endif
                        {
                            aec_config.alc_off_codec_adc_gain_r += 2;
                            mprintf("now_pga_gain_ref =%d\r\n",aec_config.alc_off_codec_adc_gain_r);
                            cm_set_codec_adc_gain(HOST_MIC_RECORD_CODEC_ID, CM_CHA_RIGHT, aec_config.alc_off_codec_adc_gain_r);
                        }
                        prompt_play_by_voice_id(GET_AEC_GAIN_AUDIO_ID, NULL, true);  //播放测试音频
                    }
                    else if(sum_ref_eng/frame_cnt >= ENERGE_UP_THR)
                    {
                        #if !REF_IN_FROM_INNER_CODEC
                        if(audio_capture.mic_channel_num == 2)
                        {
                            es7243e_init_gain -= 2.0f;
                            mprintf("now_pga_gain_ref =%d\r\n",(int32_t)es7243e_init_gain);
                            es7243e_alc_gate_en(ES7243E_ALC_ON);
                            es7243e_alc_maxgain_set(es7243e_init_gain);
                            es7243e_alc_gate_en(ES7243E_ALC_OFF);
                        }
                        else
                        #endif
                        {
                            aec_config.alc_off_codec_adc_gain_r -= 2;
                            mprintf("now_pga_gain_ref =%d\r\n",aec_config.alc_off_codec_adc_gain_r);
                            cm_set_codec_adc_gain(HOST_MIC_RECORD_CODEC_ID, CM_CHA_RIGHT, aec_config.alc_off_codec_adc_gain_r);
                        }
                        prompt_play_by_voice_id(GET_AEC_GAIN_AUDIO_ID, NULL, true);  //播放测试音频
                    }
                    else
                    {
                        gCiasAiotFuncParam.aec_gain_get_state = GET_AEC_GAIN_REF_THR;
                        err_cnt = 0;
                        mprintf("\r\n-----------------GET_AEC_GAIN_REF_THR START-----------------\r\n");
                        audio_play_set_vol_gain(67*3/VOLUME_MAX + 7);
                        prompt_play_by_voice_id(GET_AEC_REF_THR_AUDIO_ID, NULL, true);  //播放测试音频
                    }
                    sum_mic_eng =0;
                    frame_cnt =0;
                    cnt = 0;
                    
                }
                else
                {
                    err_cnt++;
                    break;
                }
                calc_flag = false;
            }  
        }
        if(gCiasAiotFuncParam.aec_gain_get_state == GET_AEC_GAIN_REF_THR)
        {
            if (CI_SS_PLAY_STATE_PLAYING == ciss_get(CI_SS_PLAY_STATE))
            {
                calc_flag = true;
                if(cnt >= 60 && frame_cnt<150)
                {
                    if (aec_enable_thr < 0.001f)
                    {
                        aec_enable_thr = p_wrapfft_audio->cur_aec_ref_eng;
                    }
                    if(aec_enable_thr > p_wrapfft_audio->cur_aec_ref_eng)
                    {
                        aec_enable_thr = p_wrapfft_audio->cur_aec_ref_eng;
                    }
                    frame_cnt++;
                }
                cnt++;
            }
            else if(CI_SS_PLAY_STATE_IDLE == ciss_get(CI_SS_PLAY_STATE) && calc_flag)
            {
                if(aec_enable_thr > 0.001f)
                {
                    mprintf("\r\n-----------------GET_AEC_PARAM_RESULT-----------------\r\n");
                    
                    uint8_t ref_gain = 0;
                    #if !REF_IN_FROM_INNER_CODEC
                    if(audio_capture.mic_channel_num == 2)
                    {
                        mprintf("SET AEC_PGA_GAIN_L、AEC_PGA_GAIN_R = %d \r\n",aec_config.alc_off_codec_adc_gain_l);
                        mprintf("SET OUTSIDE_CODEC_GAIN = %d\r\n",(int32_t)es7243e_init_gain);
                        ref_gain = (int8_t)es7243e_init_gain;
                    }
                    else
                    #endif
                    {
                        mprintf("SET AEC_PGA_GAIN_L = %d \r\n",aec_config.alc_off_codec_adc_gain_l);
                        mprintf("SET AEC_PGA_GAIN_R = %d\r\n",aec_config.alc_off_codec_adc_gain_r);
                        ref_gain = (int8_t)aec_config.alc_off_codec_adc_gain_r;
                    }
                    mprintf("SET VOLUME_DEFAULT = %d\r\n",volume);
                    mprintf("SET AEC_REF_ENG_MIN_THR =%d\r\n",(int32_t)aec_enable_thr);
                    uint8_t gain_rslt[7] = {0};
                    gain_rslt[0] = aec_config.alc_off_codec_adc_gain_l;
                    gain_rslt[1] = ref_gain;
                    gain_rslt[2] = volume;
                    gain_rslt[3] = ((int32_t)aec_enable_thr & 0xff);
                    gain_rslt[4] = (((int32_t)aec_enable_thr >> 8) & 0xff);
                    gain_rslt[5] = (((int32_t)aec_enable_thr >> 16) & 0xff);
                    gain_rslt[6] = (((int32_t)aec_enable_thr >> 24) & 0xff);
                    cias_send_cmd_and_data(CIAS_AEC_GAIN_RESULT, &gain_rslt, 7, DEF_FILL);
                    gCiasAiotFuncParam.aec_gain_get_state = GET_AEC_GAIN_IDLE;
                    err_cnt = 0;
                    frame_cnt =0;
                    cnt = 0;
                    aec_enable_thr = 0.0f;
                    audio_play_set_vol_gain(67*volume/VOLUME_MAX + 7);
                }
                else
                {
                    err_cnt++;
                    break;
                }
                calc_flag = false;
            }
        }
    }while (0);

    if(gCiasAiotFuncParam.aec_gain_get_state != GET_AEC_GAIN_IDLE)
    {
        if(err_cnt >= 3)
        {
            mprintf("aec test err\r\n");
            test_rslt = 0;
            err_cnt = 0;
            frame_cnt =0;
            cnt = 0;
            aec_enable_thr = 0.0f;
            cias_send_cmd_and_data(CIAS_AEC_GAIN_RESULT, &test_rslt, 1, DEF_FILL);
            gCiasAiotFuncParam.aec_gain_get_state = GET_AEC_GAIN_IDLE;
        }
        else if(err_cnt != 0 && calc_flag && (CI_SS_PLAY_STATE_IDLE == ciss_get(CI_SS_PLAY_STATE)))
        {
            if(gCiasAiotFuncParam.aec_gain_get_state == GET_AEC_GAIN_MIC || gCiasAiotFuncParam.aec_gain_get_state == GET_AEC_GAIN_REF)
            prompt_play_by_voice_id(GET_AEC_GAIN_AUDIO_ID, NULL, true);  //播放测试音频
            if(gCiasAiotFuncParam.aec_gain_get_state == GET_AEC_GAIN_REF_THR)
            prompt_play_by_voice_id(GET_AEC_REF_THR_AUDIO_ID, NULL, true);  //播放测试音频
            calc_flag = false;
        }
        update_awake_time(); // 更新本地唤醒时间
    }
    
}
#endif


/*****************************************4路音频通道串口采音工具*******************************************/

/* 4路音频经 G.722(64kbps, 16kHz宽带ADPCM) 压缩后串口输出，需配合 DEBUG_AUDIO_4CH_UART_UPLOAD_EN=1。
   4路原始≈128KB/s，G.722压缩后 4×64kbps≈32.8KB/s，921600波特率占用约36%。
   定点ADPCM编码开销极低且无需跨帧累积(256点/帧直接编码)，每路256点→128B固定长。
   压缩帧为变长子帧格式:
     [magic(4B)=0x5AA51D4C][seq(2B)][payload_len(2B)][4×[1B长度(恒128)][128B数据]] = 524B/16ms
   PC端: python uart_4ch_audio_parser.py --codec g722 解析出 .g722 裸码流,
 */



/********** (C) COPYRIGHT Chipintelli Technology Co., Ltd. *****END OF FILE****/
