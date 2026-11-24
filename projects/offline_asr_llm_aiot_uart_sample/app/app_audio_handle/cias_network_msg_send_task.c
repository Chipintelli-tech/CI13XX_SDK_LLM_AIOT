#include <stdlib.h>
#include <stdint.h>
#include "system_msg_deal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"
#include "ci_log.h"
#include "sdk_default_config.h"
#include "cias_network_msg_protocol.h"
#include "cias_network_msg_send_task.h"
#include "ci130x_uart.h"
#include "ci130x_core_misc.h"
#include "cias_uart_protocol.h"
#include "cias_audio_data_handle.h"
#include "cias_common.h"
#include "user_config.h"
#if WIFI_CMD_BY_UART || AUDIO_DATA_PLAY_BY_UART || AUDIO_DATA_UPLOAD_BY_UART

uint8_t broad_link_buf[NETWORK_SEND_BUFF_NUM * NETWORK_SEND_BUFF_MAX_SIZE] = {0}; // 
static QueueHandle_t network_msg_queue = NULL;
static SemaphoreHandle_t broad_link_free_sem = NULL;  //broad_link_buf空闲槽位计数信号量，消费者发送完成后归还
static uint8_t g_broad_link_slot_index = 0;           //broad_link_buf统一轮转槽位索引（三个打包函数共用）
SemaphoreHandle_t network_send_sem =NULL;
bool network_send_task_init(void)
{
#if NET_AUDIO_PLAY_BY_PCM && AUDIO_COMPRESS_RECORD_DISABLE   //精简播放播放pcm需要更大的消息队列，不然播放会卡顿
    network_msg_queue = xQueueCreate(8, sizeof(sdio_task_msg_t));
#else
    network_msg_queue = xQueueCreate(4, sizeof(sdio_task_msg_t));
#endif
    if(!network_msg_queue)
    {
        mprintf("network_msg_queue creat fail\r\n");
        return false;
    }
	network_send_sem = xSemaphoreCreateMutex();
    if (NULL == network_send_sem)
    {
        mprintf("network_MuxSem creat fail\r\n");
        return false;
    }
    broad_link_free_sem = xSemaphoreCreateCounting(NETWORK_SEND_BUFF_NUM, NETWORK_SEND_BUFF_NUM);
    if (NULL == broad_link_free_sem)
    {
        mprintf("broad_link_free_sem creat fail\r\n");
        return false;
    }
    return true;
}

/**
 * @brief 申请一块broad_link_buf发送槽位
 *        计数信号量代表空闲槽位，消费者发送完成后归还；链路恢复后计数自然回补，可自愈，
 *        不会再出现旧方案中裸计数因多任务竞争错账而永久报overwrite的问题
 *
 * @param acquired : pdPASS表示正常申请到槽位（入队失败时需调用broad_link_buf_release归还）；
 *                   pdFAIL表示16ms内无空闲槽位，将覆盖最旧槽位，保持历史兼容行为，此时不可归还
 * @return 槽位起始指针
 */
static uint8_t *broad_link_buf_acquire(BaseType_t *acquired)
{
    uint8_t slot;
    //目前在VAD开始时，pcm_compress_stream_buffer有需回退的数据，会频繁写入broad_link_buf，等待时间约1帧
    //（VAD START后pcm_compress_stream_buffer会释放PCM_MSG_STREAM_NUM - NN_VAD_FRONT_PORCH帧数据,该超时时间可结合应用场景在不丢帧的情况下给）
    *acquired = xSemaphoreTake(broad_link_free_sem, pdMS_TO_TICKS(16));
    if (pdPASS != *acquired)
    {
        //若出现上传的音频叠音的情况，此处解决办法：1、加大NETWORK_SEND_BUFF_NUM 2、减小network_send_data_task任务的等待时间
        mprintf("ERR: broad_link_buf overwrite,please set NETWORK_SEND_BUFF_NUM large\n");
    }
    /*三个打包函数共用同一套槽位，轮转索引的自增必须原子，避免多任务选中同一块buffer互相覆盖*/
    portENTER_CRITICAL();
    slot = g_broad_link_slot_index;
    g_broad_link_slot_index++;
    if (g_broad_link_slot_index >= NETWORK_SEND_BUFF_NUM)
    {
        g_broad_link_slot_index = 0;
    }
    portEXIT_CRITICAL();
    return broad_link_buf + slot * NETWORK_SEND_BUFF_MAX_SIZE;
}

static void broad_link_buf_release(BaseType_t acquired)
{
    if (pdPASS == acquired)
    {
        xSemaphoreGive(broad_link_free_sem);
    }
}

/**
 * @brief A simple code for other component send system message to this module, just wrap freertos queue function
 * 
 * @param flag_from_isr : 0 call this function not from isr, other call this from isr
 * @param send_msg : system message
 * @param xHigherPriorityTaskWoken : if call this not from isr set NULL
 * @return BaseType_t 
 */
BaseType_t send_msg_to_network_task(sdio_task_msg_t *send_msg, BaseType_t *xHigherPriorityTaskWoken)
{
    //if(0 != __get_IPSR())
    if(0 != check_curr_trap())
    {
        return xQueueSendFromISR(network_msg_queue,send_msg, xHigherPriorityTaskWoken);
    }
    else
    {
        return xQueueSend(network_msg_queue, send_msg, 100);
    }
}


int32_t broadlink_frame_create(uint32_t type, const uint8_t *input, uint32_t len, uint8_t *output, uint32_t maxsize, uint32_t fill_data, uint16_t index)
{
    cias_standard_head_t *head = (cias_standard_head_t *)output;
    static uint16_t index_packg = 0;
#if AUDIO_SEND_WITH_PROTOCOL_HEADER
    /* Check param */
    if (output == NULL || len + sizeof(cias_standard_head_t) > maxsize)
    {
        ci_logerr(LOG_MEDIA,"broadlink_frame_create err\n");
        return -1;
    }
    /* DNA standard header initialization */
    memset(head, 0, sizeof(cias_standard_head_t));
    head->magic = CIAS_STANDARD_MAGIC;  
    head->type = type;
    head->len = len;
    head->version = 0x100;   
    for(int i = 0; i < len; i++)
    {
        head->checksum += input[i];
    }
    if(type == PCM_MIDDLE)
    {
        head->version = index_packg;
        index_packg++;
    }
    else
        index_packg = 0;
    
    head->fill_data = fill_data;
    /* Fill data part */
    memcpy(output + sizeof(cias_standard_head_t), input, len);
    return head->len + sizeof(cias_standard_head_t);
#else
    memcpy(output, input, len);
    return head->len;
#endif
}


int cais_send_msg_flag_to_network(int8_t *send_buffer)
{
    network_send(send_buffer,strlen(send_buffer));
}

int32_t cias_send_cmd(uint32_t cmd, uint32_t fill_data)
{
#if !AUDIO_SEND_WITH_PROTOCOL_HEADER
    return 0;
#endif
    uint8_t send_msg_buf[16] = {0};
    sdio_task_msg_t send_msg;
    broadlink_frame_create(cmd, NULL, 0, send_msg_buf, 16, fill_data, 0);
    memcpy((void *)send_msg.network_data.cmd, send_msg_buf, 16);
    send_msg.network_data.type = ONLY_CMD;
    send_msg.msg_type = SDIO_DMA_SEND;
    send_msg.network_data.length = 16;
    send_msg.network_data.flag = 0;
    if (pdPASS != send_msg_to_network_task(&send_msg, NULL))
    {
        mprintf("cias_send_cmd err:%d\n", __LINE__);
    }
    return cmd;
}

/**
 * @brief
 *
 * @param packet_type 数据包类型
 * @param voice_data 音频数据
 * @param packet_len 数据长度
 * @return int -1 错误， 返回包长度
 */
int32_t cias_send_cmd_and_data(uint32_t cmd_type, uint8_t *buf, int32_t len, uint32_t fill_data)
{
#if !AUDIO_SEND_WITH_PROTOCOL_HEADER
    return 0;
#endif
    int32_t package_length = 0;
    sdio_task_msg_t send_msg;
    BaseType_t acquired = pdFAIL;
    uint8_t *slot_buf = broad_link_buf_acquire(&acquired);

    package_length = broadlink_frame_create(cmd_type, buf, len, slot_buf, NETWORK_SEND_BUFF_MAX_SIZE, fill_data, 0);
    send_msg.network_data.payload = (uint32_t)slot_buf;
    send_msg.msg_type = SDIO_DMA_SEND;
    send_msg.network_data.type = CMD_AND_DATA;
    send_msg.network_data.length = package_length;
    if (cmd_need_ack(cmd_type) == true)
    {
        send_msg.network_data.flag = 1; /*需要ACK*/
    }
    else
    {
        send_msg.network_data.flag = 0; /*不需要ACK*/
    }
    if (pdPASS != send_msg_to_network_task(&send_msg, NULL))
    {
        mprintf("sdio err3\n");
        broad_link_buf_release(acquired); /*入队失败，归还已申请的槽位*/
        return -1;
    }
    return package_length;
}


/**
 * @brief 打包发送音频数据
 * 
 * @param packet_type 数据包类型
 * @param voice_data 音频数据
 * @param packet_len 数据长度
 * @return int -1 错误， 返回包长度
 */
int voice_data_packet_and_send(uint32_t packet_type,uint8_t *voice_data, int32_t packet_len)
{
    sdio_task_msg_t send_msg;
    BaseType_t acquired = pdFAIL;
    uint8_t *slot_buf = broad_link_buf_acquire(&acquired);

    broadlink_frame_create(packet_type, voice_data, packet_len, slot_buf, NETWORK_SEND_BUFF_MAX_SIZE, 0x12345678,0);
    send_msg.network_data.payload = (uint32_t)slot_buf;
    send_msg.msg_type = SDIO_DMA_SEND;
    send_msg.network_data.type = AUDIO_MIDDLE;
#if AUDIO_SEND_WITH_PROTOCOL_HEADER
    send_msg.network_data.length = packet_len + 16;
#else
    send_msg.network_data.length = packet_len;
#endif
    send_msg.network_data.flag = 0;
    if(pdPASS != send_msg_to_network_task(&send_msg,NULL))
    {
        mprintf("sdio err3\n");
        broad_link_buf_release(acquired); /*入队失败，归还已申请的槽位*/
        return -1;
    }

    return 0;
}

/**
 * @brief 打包发送cjson
 * 
 * @param packet_type   数据包类型
 * @param data          cjson数据
 * @param packet_len    数据长度
 * @return int -1 错误，返回包长度
 */
int cias_cjson_packet_and_send(uint32_t packet_type,uint8_t *data, int32_t packet_len)
{
    sdio_task_msg_t send_msg;
    BaseType_t acquired = pdFAIL;
    uint8_t *slot_buf = broad_link_buf_acquire(&acquired);

    broadlink_frame_create(CIAS_CJSON_DATA, data, packet_len, slot_buf, NETWORK_SEND_BUFF_MAX_SIZE, 0x12345678,0);
    send_msg.network_data.payload = (uint32_t)slot_buf;
    send_msg.msg_type = SDIO_DMA_SEND;
    send_msg.network_data.type = AUDIO_MIDDLE;
    send_msg.network_data.length = packet_len + 16;
    send_msg.network_data.flag = 0;

    if(pdPASS != send_msg_to_network_task(&send_msg,NULL))
    {
        mprintf("sdio err3\n");
        broad_link_buf_release(acquired); /*入队失败，归还已申请的槽位*/
        return -1;
    }
    return 0;
}


/**
 * @brief 串口发送任务
 * 
 * @param p_arg 
 */
void network_send_data_task(void *p_arg)
{
    sdio_task_msg_t rev_msg;
    BaseType_t ret = pdPASS;
    network_msg_data_t *data = NULL;
    while(1)
    {
        ret = xQueueReceive(network_msg_queue, &rev_msg, portMAX_DELAY);
        if(pdPASS == ret)
        {
           // ci_loginfo(LOG_USER,"rev_msg.msg_type = %d\r\n", rev_msg.msg_type);
            switch (rev_msg.msg_type)
            {
                case SDIO_DMA_SEND:
                {
                    data = &rev_msg.network_data;
                    //mprintf("data->length = %d, data->type = %d\r\n", data->length, data->type);
                    if(data->type == CMD_AND_DATA)
                    {
                        ret = network_send((int8_t *)data->payload, data->length);
                    }
                    else if(data->type == ONLY_CMD)
                    {
                        ret = network_send((int8_t *)data->cmd, data->length);
                    }
                    else
                    {
                       ret = network_send((int8_t *)data->payload, data->length);
                    }
                    /*ONLY_CMD使用消息体内嵌cmd，不占用broad_link_buf；其余消息发送完成后归还槽位*/
                    if(data->type != ONLY_CMD)
                    {
                        xSemaphoreGive(broad_link_free_sem);
                    }
                    break;
                }
                default:
                {

                    break;
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(2));    //发送加延时，防止接收端串口buffer溢出
    }
}
#else
int32_t cias_send_cmd(uint32_t cmd, uint32_t fill_data)
{
    return 0;
}
int32_t cias_send_cmd_and_data(uint32_t cmd_type, uint8_t *buf, int32_t len, uint32_t fill_data)
{
    return 0;
}
int voice_data_packet_and_send(uint32_t packet_type,uint8_t *voice_data, int32_t packet_len)
{
    return 0;
}
int cias_cjson_packet_and_send(uint32_t packet_type,uint8_t *data, int32_t packet_len)
{
    return 0;
}
#endif
