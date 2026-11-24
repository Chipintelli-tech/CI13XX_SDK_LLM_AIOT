#ifndef __UART_DUMP_TOOLS_H
#define __UART_DUMP_TOOLS_H

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

void uart_send_voice_init(void);
void audio_debug_uart0_upload(int16_t *left, int16_t *right, ci_wrapfft_audio *p_wrapfft_audio);
void audio_debug_uart_4ch_upload(ci_wrapfft_audio *p_wrapfft_audio);

#ifdef __cplusplus
}
#endif

#endif
/***************** (C) COPYRIGHT Chipintelli Technology Co., Ltd. *****END OF FILE****/





