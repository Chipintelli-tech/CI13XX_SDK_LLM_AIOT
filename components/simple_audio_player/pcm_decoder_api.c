#include "user_config.h"
#if	SIMPLE_AUDIO_PLAYER_ENABLE
#include "ci_log.h"
#include "simple_audio_player.h"




static void *decoder_init(void)
{
}

static void docoder_deinit(void* decoder_haldle)
{
}

static int get_info(void* decoder_handle, uint8_t *data, int *bytes_left, uint8_t *pcm_buf, audio_format_info_t *format_info)
{
    format_info->samprate = NET_AUDIO_PCM_G722_SAMPLE_RATE;
    format_info->bits_per_sample = NET_AUDIO_PCM_G722_SAMPLE_DEPTH;
    format_info->channels = NET_AUDIO_PCM_G722_CHANNEL_FLAG;
    format_info->samples_per_frame = NET_AUDIO_PCM_FRAME_SIZE/sizeof(short);
    format_info->src_data_size = 0;
    return 0;
}

static int decode_one_frame(void* decoder_handle, uint8_t *data, int *bytes_left, uint8_t *pcm_buf, uint32_t *pcm_data_size)
{
    int ret = 0;
    if (*bytes_left >= NET_AUDIO_PCM_FRAME_SIZE)
    {
        mprintf("==decode_one_frame len= %d\r\n",*bytes_left);
        memcpy(pcm_buf, data, NET_AUDIO_PCM_FRAME_SIZE);
        *pcm_data_size = NET_AUDIO_PCM_FRAME_SIZE;
        *bytes_left -= NET_AUDIO_PCM_FRAME_SIZE;
    }
    else
    {
        ret = -1;
    }
    return ret;
}

#if NET_AUDIO_PLAY_BY_PCM                          
register_audio_decoder(pcm, decoder_init, docoder_deinit, get_info, decode_one_frame); 
#endif
#endif
