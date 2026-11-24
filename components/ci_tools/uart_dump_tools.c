#include "codec_manager.h"
#include <string.h>
#include "board.h"
#include "ci130x_gpio.h"
#include <stdbool.h>
#include "FreeRTOS.h"
#include "task.h"
#include "sdk_default_config.h"
#include "ci130x_uart.h"
#include "ci130x_dma.h"
#include "stream_buffer.h"
#include "status_share.h"
#include "user_config.h"
#include "uart_dump_tools.h"
#if DEBUG_AUDIO_UART_UPLOAD_EN
#if DEBUG_AUDIO_4CH_G722_EN
#include "g722_enc_dec.h"       /* WebRtc G.722 编码器(WebRtc_g722_encode_*) */
#endif

/*****************************************4路音频通道串口采音工具*******************************************/

/* 4路音频经 G.722(64kbps, 16kHz宽带ADPCM) 压缩后串口输出，需配合 DEBUG_AUDIO_4CH_UART_UPLOAD_EN=1。
   4路原始≈128KB/s，G.722压缩后 4×64kbps≈32.8KB/s，921600波特率占用约36%。
   定点ADPCM编码开销极低且无需跨帧累积(256点/帧直接编码)，每路256点→128B固定长。
   压缩帧为变长子帧格式:
     [magic(4B)=0x5AA51D4C][seq(2B)][payload_len(2B)][4×[1B长度(恒128)][128B数据]] = 524B/16ms
   PC端: python uart_4ch_audio_parser.py --codec g722 解析出 .g722 裸码流,
 */

uint32_t tmp_voice_addr = 0;
StreamBufferHandle_t gUartRecordStreamBuffer = NULL; // 串口采音队列

/* 4路采音帧定义: 8字节帧头(magic+seq+len) + 4通道×256点×2字节 */
#define DEBUG_AUDIO_FRAME_HEADER_SIZE    8
#define DEBUG_AUDIO_FRAME_MAGIC          0x5AA51D4C
#define DEBUG_AUDIO_CH_NUM               (4)
#define DEBUG_AUDIO_4CH_FRAME_SIZE       (AUDIO_CAP_POINT_NUM_PER_FRM * DEBUG_AUDIO_CH_NUM * sizeof(int16_t) + DEBUG_AUDIO_FRAME_HEADER_SIZE)
/* 2路(L/R交织)采音帧定义: 无帧头, 256点×2通道×2字节 (= block_size) */
#define DEBUG_AUDIO_2CH_FRAME_SIZE       (AUDIO_CAP_POINT_NUM_PER_FRM * 2 * sizeof(int16_t))

#if DEBUG_AUDIO_4CH_G722_EN
/* ---- G.722 压缩(4路) ------------------------------------------------------------
   G.722(64kbps, 16kHz宽带ADPCM)无需跨帧累积, 每个原始帧256点/路直接编码。
   64kbps下 2个输入样本→1个输出字节, 故每路 256点→128B 固定长。
   压缩帧为变长子帧格式(与PC端解析器 --codec g722 的子帧切分逻辑一致):
     [magic(4B)][seq(2B)][payload_len(2B)][ 4×[1B长度(恒=128)][128B数据] ] = 524B/16ms ≈ 32.8KB/s */
#define G722_4CH_BYTES_PER_CH          (AUDIO_CAP_POINT_NUM_PER_FRM / 2)   /* 每路每帧压缩后字节数(128B) */
#define DEBUG_AUDIO_4CH_G722_FRAME_MAX (DEBUG_AUDIO_FRAME_HEADER_SIZE + DEBUG_AUDIO_CH_NUM * (1 + G722_4CH_BYTES_PER_CH))  /* 压缩帧总长(524B) */
extern int get_heap_bytes_remaining_size(void);  /* 初始化时打印堆余量, 便于核对内存占用 */
#endif
/* 轮询等待上一帧DMA完成: DMA传输结束后硬件自动清除通道使能位,
   不依赖中断回调链路(dma_without_os/os两版本回调行为不一致)。
   2056B@2M约10.3ms, 25ms仍未完成视为硬件异常 */
static void uart_dma_wait_idle(void)
{
    uint32_t wait = 0;
    while (DMAC_ChannelEnableStatus(DMACChannel1))
    {
        vTaskDelay(pdMS_TO_TICKS(1));
        if (++wait > 25)
        {
            break;
        }
    }
}

volatile uint8_t uart_dma_trans_done = 1;

static void uart_dma_read_irq_callback(void)
{
    uart_dma_trans_done = 1;
}
void audio_pre_rslt_write_data_from_uart(uint32_t addr, uint32_t size)
{
    // init_timer0();
    // timer0_start_count();
    if (!uart_dma_trans_done)
    {
        // mprintf("pre voice data overflow\n");
    }
    uart_dma_trans_done = 0;
    extern void set_dma_int_callback(DMACChannelx dmachannel, dma_callback_func_ptr_t func);
    set_dma_int_callback(DMACChannel1, uart_dma_read_irq_callback);

    DMAC_Peripherals uart_dma_peripherals_num = DMAC_Peripherals_UART0_TX;
    uint32_t uart_fifo_addr = UART0FIFO_BASE;
    if (((uint32_t)USE_UART_SEND_PRE_RSLT_AUDIO_NUMBER == (uint32_t)UART1))
    {
        uart_dma_peripherals_num = DMAC_Peripherals_UART1_TX;
        uart_fifo_addr = UART1FIFO_BASE;
    }
    else if (((uint32_t)USE_UART_SEND_PRE_RSLT_AUDIO_NUMBER == (uint32_t)UART2))
    {
        uart_fifo_addr = UART2FIFO_BASE;
        uart_dma_peripherals_num = DMAC_Peripherals_UART2_TX;
    }

    DMAC_M2P_P2M_advance_config(DMACChannel1, uart_dma_peripherals_num, M2P_DMA, addr, uart_fifo_addr, size,
                                TRANSFERWIDTH_8b, BURSTSIZE1, DMAC_AHBMaster1);
    // while(!uart_dma_trans_done);
    // uart_dma_trans_done = 0;
    // timer0_end_count_only_print_time_us();
}

#if DEBUG_AUDIO_4CH_G722_EN
/*============================================================================
 * [新增] 4路 G.722 编码与组帧发送
 *
 * 数据流: audio_debug_uart_4ch_upload(ISR侧: 4路交织PCM原始帧2056B入队)
 *         → uart_record_task(收帧+校验) → g722_encode_and_send(逐路编码、组帧、DMA发出)
 * 编码器与缓冲全部静态分配，不占任务栈；编码器状态由库内 pvPortMalloc(~0.4KB/路)。
 *==========================================================================*/

/*---------------- 功能代码：编码器状态与帧缓冲 ----------------*/
static G722EncoderState *s_g722_enc[DEBUG_AUDIO_CH_NUM];        /* 每路一个独立编码器状态 */
static uint8_t  s_g722_tx[DEBUG_AUDIO_4CH_G722_FRAME_MAX];      /* 组装好的压缩发送帧(524B) */
static uint8_t  s_g722_tmp[G722_4CH_BYTES_PER_CH];              /* 单路编码输出暂存(128B) */
static int16_t  s_g722_pcm[AUDIO_CAP_POINT_NUM_PER_FRM];        /* 单路解交织暂存(256点) */
static uint16_t s_g722_seq;                                     /* 压缩帧序号(任务侧自建, PC端检帧用) */
static uint8_t  s_g722_ready;                                   /* 编码器初始化完成标志, 0=任务侧丢帧避免乱码输出 */


/* 初始化4个 G.722 编码器(64kbps, 16kHz)。状态由库内部 pvPortMalloc, 每路约0.4KB, 占用FreeRTOS堆极小。
   任一路失败时释放已建编码器并保持 ready=0, 任务侧将直接丢帧, 不会输出乱码 */
static void debug_4ch_g722_init(void)
{
    mprintf("[4ch g722] heap before=%d\r\n", get_heap_bytes_remaining_size());
    for (int c = 0; c < DEBUG_AUDIO_CH_NUM; c++)
    {
        s_g722_enc[c] = WebRtc_g722_encode_init(NULL, G722_BITRATE_64k, 0);
        if (!s_g722_enc[c])
        {
            mprintf("[4ch g722] encoder %d init fail\r\n", c);
            for (int k = 0; k < c; k++)
            {
                WebRtc_g722_encode_release(s_g722_enc[k]);
                s_g722_enc[k] = NULL;
            }
            return;
        }
    }

    s_g722_seq = 0;
    s_g722_ready = 1;
    mprintf("[4ch g722] ready, heap after=%d\r\n", get_heap_bytes_remaining_size());
}

/* 把一个原始帧(4路交织, 256点/路)逐路解交织并G.722编码, 组帧后DMA发出。
   帧格式: 8B帧头 + 4×[1B长度(恒128)][128B数据] = 524B */
static void g722_encode_and_send(const int8_t *rx_frame)
{
    const int16_t *raw = (const int16_t *)(rx_frame + DEBUG_AUDIO_FRAME_HEADER_SIZE);
    uint8_t *p = s_g722_tx;
    *(uint32_t *)p = DEBUG_AUDIO_FRAME_MAGIC;
    p += 4;
    *(uint16_t *)p = s_g722_seq++;
    p += 2;
    uint8_t *plen_p = p;
    p += 2;

    for (int c = 0; c < DEBUG_AUDIO_CH_NUM; c++)
    {
        /* 解交织: 交织序 M,R,REF,DST → 单路连续PCM */
        for (int i = 0; i < AUDIO_CAP_POINT_NUM_PER_FRM; i++)
        {
            s_g722_pcm[i] = raw[DEBUG_AUDIO_CH_NUM * i + c];
        }
        size_t n = WebRtc_g722_encode(s_g722_enc[c], s_g722_tmp, s_g722_pcm, AUDIO_CAP_POINT_NUM_PER_FRM);
        *p++ = (uint8_t)n;              /* 子帧长度前缀: 64kbps下恒为128 */
        memcpy(p, s_g722_tmp, n);
        p += n;
    }

    *(uint16_t *)plen_p = (uint16_t)(p - (plen_p + 2));

    uart_dma_wait_idle();
    audio_pre_rslt_write_data_from_uart((uint32_t)s_g722_tx, (uint32_t)(p - s_g722_tx));
}
#endif /* DEBUG_AUDIO_4CH_G722_EN */

// 串口录音任务
void uart_record_task(void *p_arg)
{
    uint16_t stream_aviable_len = 0;
    /* 帧缓冲放静态区而非栈上, 避免任务栈被迫开到9KB+, 浪费堆内存 */
#if DEBUG_AUDIO_4CH_UART_UPLOAD_EN
    static int8_t record_rx_temp[DEBUG_AUDIO_4CH_FRAME_SIZE];
    uint16_t frame_size = DEBUG_AUDIO_4CH_FRAME_SIZE;
#else
    static int8_t record_rx_temp[DEBUG_AUDIO_2CH_FRAME_SIZE];
    uint16_t frame_size = DEBUG_AUDIO_2CH_FRAME_SIZE;
#endif
    while (1)
    {
        stream_aviable_len = xStreamBufferBytesAvailable(gUartRecordStreamBuffer);
        if (stream_aviable_len < frame_size)
        {
            vTaskDelay(pdMS_TO_TICKS(1));
            continue;
        }

        uart_dma_wait_idle();
        memset(record_rx_temp, 0, frame_size);
        int rx_size = xStreamBufferReceive(gUartRecordStreamBuffer, record_rx_temp, frame_size, 0);
        if (rx_size != frame_size)
        {
            continue;
        }

#if DEBUG_AUDIO_4CH_G722_EN
        /* 4路G.722压缩: 每个原始帧直接逐路编码发出, 无跨帧累积 */
        if (!s_g722_ready)
        {
            continue;
        }
        /* 校验原始帧magic/payload_len, 防止半帧或错帧数据进入编码器
           (压缩帧seq由任务侧自建, 上游seq仅用于流内自描述) */
        if (*(uint32_t *)record_rx_temp != DEBUG_AUDIO_FRAME_MAGIC)
        {
            continue;
        }
        if (*(uint16_t *)(record_rx_temp + 6) != AUDIO_CAP_POINT_NUM_PER_FRM * DEBUG_AUDIO_CH_NUM * sizeof(int16_t))
        {
            continue;
        }
        g722_encode_and_send(record_rx_temp);
#else
        /* 非4路压缩模式: 直接把收到的整帧经DMA发出, 恢复原始SDK直传行为
           (2路=1024B交织L/R; 4路未压缩=2056B带帧头), 与 audio_debug_uart0_upload/
           audio_debug_uart_4ch_upload 入队格式严格对应 */
        audio_pre_rslt_write_data_from_uart((uint32_t)record_rx_temp, (uint32_t)rx_size);
#endif
    }
}
void uart_send_voice_init(void)
{
    /* 4路帧缓冲放静态BSS区, 不占FreeRTOS堆 */
    static uint8_t s_tmp_voice_buf[DEBUG_AUDIO_4CH_FRAME_SIZE];
    tmp_voice_addr = (uint32_t)s_tmp_voice_buf;
    UARTDMAConfig((UART_TypeDef *)USE_UART_SEND_PRE_RSLT_AUDIO_NUMBER, USE_UART_SEND_PRE_RSLT_AUDIO_BAUD);
    gUartRecordStreamBuffer = xStreamBufferCreate(USE_UART_SEND_PRE_RSLT_AUDIO_BUF_LEN, 1);
    if(gUartRecordStreamBuffer)
    {
        #if DEBUG_AUDIO_4CH_G722_EN
        debug_4ch_g722_init();
        #endif

        /* 4路帧接收缓冲 + 编码组帧, 任务栈 1024 words(4KB) */
        const uint16_t task_stack = 1024;
        /* 优先级4: 遵守系统约定(优先级5为Tmr_svc独占), 同步采用DMA使能位轮询, 不依赖中断回调 */
        if (xTaskCreate(uart_record_task, "uart_record_task", task_stack, NULL, 4, NULL) != pdPASS)
        {
            mprintf("uart record task create error\r\n");
        }
    }
    else
    {
        mprintf("uart record init error\r\n");
    }
}

#if DEBUG_AUDIO_4CH_UART_UPLOAD_EN
/**
 * @brief 4路音频采音经UART输出(MICL/MICR/REF/DST1)
 *
 * @param p_wrapfft_audio 前端音频数据流句柄,含mic[0]/mic[1]/ref[0]/dst[0]
 * 本函数仅把4路交织PCM入队到 gUartRecordStreamBuffer:
 *   入队格式: [magic(4B)][seq(2B)][payload_len(2B)][4ch交织PCM 2048B] = 2056B/帧
 * 实际UART发送由 uart_record_task 完成: 未压缩时直发上述2056B原始帧(需2M波特率);
 *   开启 DEBUG_AUDIO_4CH_G722_EN 时先做4路G.722压缩, 发送变长压缩帧(921600波特率即可)
 * PC端按 0x5AA51D4C 扫描对齐, 4列拆分, 每列16kHz/16bit/单声道
 */
void audio_debug_uart_4ch_upload(ci_wrapfft_audio *p_wrapfft_audio)
{
    static uint16_t seq = 0;
    int16_t *pcm = (int16_t *)((uint8_t *)tmp_voice_addr + DEBUG_AUDIO_FRAME_HEADER_SIZE);
    int num = AUDIO_CAP_POINT_NUM_PER_FRM;

    /* 32k采集配置下 mic[1]/ref[0] 为32k原始数据(512点/帧), SDK仅在IIS等出口
       做2:1抽取(alg_preprocess.c iis_audio_choose), 此处按同规则拷贝时抽取,
       不原地修改源缓冲; mic[0]/dst[0] 已是16k直读 */
    bool need_decim = (32000 == p_wrapfft_audio->module_config->sample_frequency);

    /* 4路交织: ch0=MICL, ch1=MICR, ch2=REF, ch3=DST1 */
    for (int i = 0; i < num; i++)
    {
        pcm[4 * i + 0] = p_wrapfft_audio->mic[0][i];                                                  /* MICL */
        pcm[4 * i + 1] = need_decim ? p_wrapfft_audio->mic[1][2 * i] : p_wrapfft_audio->mic[1][i];   /* MICR */
        pcm[4 * i + 2] = need_decim ? p_wrapfft_audio->ref[0][2 * i] : p_wrapfft_audio->ref[0][i];   /* REFL */
        pcm[4 * i + 3] = p_wrapfft_audio->dst[0][i];                                                 /* DST1 */
    }
    /* 帧同步头 */
    *(uint32_t *)tmp_voice_addr = DEBUG_AUDIO_FRAME_MAGIC;
    *(uint16_t *)(tmp_voice_addr + 4) = seq++;
    *(uint16_t *)(tmp_voice_addr + 6) = AUDIO_CAP_POINT_NUM_PER_FRM * DEBUG_AUDIO_CH_NUM * sizeof(int16_t);

    if (gUartRecordStreamBuffer)
    {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        /* 溢出保护: 缓冲剩余空间不足整帧时丢弃整帧, 保证流中永远是完整帧,
           避免半帧数据导致PC端帧头字节混入PCM产生满幅毛刺 */
        if (xStreamBufferSpacesAvailable(gUartRecordStreamBuffer) < DEBUG_AUDIO_4CH_FRAME_SIZE)
        {
            return;
        }
        int ret = xStreamBufferSendFromISR(gUartRecordStreamBuffer, (int8_t *)tmp_voice_addr,
                                           DEBUG_AUDIO_4CH_FRAME_SIZE, &xHigherPriorityTaskWoken);
        if (ret != DEBUG_AUDIO_4CH_FRAME_SIZE)
        {
            mprintf("4ch uart stream send error, send len = %d\r\n", ret);
        }
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}
#else

void audio_debug_uart0_upload(int16_t *left, int16_t *right, ci_wrapfft_audio *p_wrapfft_audio)
{
    uint32_t block_size = AUDIO_CAP_POINT_NUM_PER_FRM * 2 * sizeof(int16_t);;
    int16_t *pcm_data_p = (int16_t *)tmp_voice_addr;
    int num = block_size / sizeof(int16_t) / 2;

    // static int cnt = 0;

    // if (cnt == 30)
    // {
    //     uart_send_voice_init();
    // }
    // else if (cnt >= 30)
    // {

        // audio_pre_rslt_write_data_from_uart((uint32_t)tmp_voice_addr,block_size);
        // #if USE_UART_SEND_PRE_RSLT_RECORD_LEFT
        // int8_t *src_addr = (int8_t *)left;
        // #else
        // int8_t *src_addr = (int8_t *)right;
        // #endif
        for (int i = 0; i < num; i++)
        {
            pcm_data_p[2 * i] = left[i];
            pcm_data_p[2 * i + 1] = right[i];
        }
        if (gUartRecordStreamBuffer)
        {
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            int ret = xStreamBufferSendFromISR(gUartRecordStreamBuffer, (int8_t *)pcm_data_p, block_size, &xHigherPriorityTaskWoken);
            if (ret != block_size)
            {
                mprintf("xSpeexRecordStreamBuffer send error, send len = %d\r\n", ret);
            }
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
    // }
    // else
    // cnt++;
}
#endif

#endif

