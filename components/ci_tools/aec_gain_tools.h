#ifndef __AEC_GAIN_TOOLS_H
#define __AEC_GAIN_TOOLS_H

#include "codec_manager.h"
#include "ci130x_gpio.h"
#include "ci130x_dpmu.h"
#include "ci130x_iis.h"
#include "ci130x_scu.h"
#include <stdbool.h>
#include "ci_audio_wrapfft.h"
#ifdef __cplusplus
 extern "C" {
#endif

void auto_get_aec_gain(ci_wrapfft_audio * p_wrapfft_audio);


#ifdef __cplusplus
}
#endif

#endif
/***************** (C) COPYRIGHT Chipintelli Technology Co., Ltd. *****END OF FILE****/





