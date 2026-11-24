/**
 * @file user_config.h
 * @version 2.0
 * @date 2026/01/22
 * @copyright Copyright (c) 2026  Chipintelli Technology Co., Ltd.
 * @brief 本文件为用户参数配置文件，可配置项：硬件参数配置，算法参数配置（前端算法参数和ASR识别算法参数）,应用参数配置
 * 本文件为用户参数配置文件，可配置功能为：硬件参数配置，算法参数配置（前端算法参数和ASR识别算法参数）， 应用参数配置
1.硬件参数配置：
 * --1.1 板级选择配置
 * --1.2 时钟/内外置1.1V电压/自适应波特率 配置
 * --1.3 双麦克时AEC的参考通路配置
 * --1.4 内置CODEC配置
 * --1.5 外置CODEC配置
 * --1.6 IIS配置
 * 2.算法参数配置：
 * --2.1 前端算法参数配置
 * ---2.1.1 GCC DOA参数配置
 * ---2.1.2 AEC参数配置
 * ---2.1.3 NN VAD参数配置
 * ---2.1.4 NN 降噪参数配置
 * --2.2 识别算法参数配置
 * --2.2.1 解码参数配置
 * --2.2.2 本地VAD参数配置
 * --2.2.3 自学习参数配置
 * 3.应用参数配置：
 * --3.1 识别中间结果打印使能
 * --3.2 日志串口配置
 * --3.3 串口采音配置
 * --3.4 语音上传和云端数据下行播放参数配置
 * --3.5 协议串口配置
 * --3.6 OTA功能使能配置
 * --3.7 AEC自动获取增益工具配置
 * --3.8 生产测试配置
 * --3.9 播放器参数配置
 * --3.10 提示播报+退出唤醒时间配置
 * --3.11 是否使用独立唤醒词模型配置
 * --3.12 两份固件动态切换应用配置
 * 4.参数配置正确性检查：sdk根据用户配置自动检查是否合理，不合理会在编译时给出提示信息，用户不用修改该模块：
 * 
 * */


#ifndef __USER_CONFIG_H__
#define __USER_CONFIG_H__
#include  "ci130x_codec.h"
#include  "ci130x_scu.h"

/**********************************************1.硬件参数配置*************************************************/

/***********************************1.1板级选择配置************************************/ 
/*板级配置更多细节请查看:https://document.chipintelli.com/硬件资料-->模块手册
chipintelli提供的部分开发板和模组，可以通过下面的宏选择，也可以参考开发板的板级配置文
件添加自定义板级配置文件*/
/*  
  USE_BOARD板级选择：
  1 - CI-D0XGS01J，端子模块，芯片型号必须设置为1302
  2 - CI-D0XGS02S，SMT模块，芯片型号必须设置为1302
  3 - CI-D06GT01D，开发版，芯片型号必须设置为1306
  4 - CI_D06GT01J, 端子模块，型号必须为设置1306-仅配置支持双mic 算法+AEC，其他配置不支持
  5 - 用户自定义
 */
#define USE_BOARD                   3
#if (USE_BOARD == 1)
#define CI_CHIP_TYPE                1302    //flash:2MB,SSOP24
#define BOARD_PORT_FILE             "CI-D02GS01J.c"
#elif (USE_BOARD == 2)
#define CI_CHIP_TYPE                1302    //flash:2MB,SSOP24
#define BOARD_PORT_FILE             "CI-D02GS02S.c"
#elif (USE_BOARD == 3)
#define CI_CHIP_TYPE                1306    //flash:4MB,QFN40
#define BOARD_PORT_FILE             "CI-D06GT01D.c"
#elif (USE_BOARD == 4)
#define CI_CHIP_TYPE                1306    //flash:4MB,QFN40 双mipc算法+外部codec 7243e用该板级
#define BOARD_PORT_FILE             "CI-D06GT01J.c"
#elif (USE_BOARD == 5)
#define CI_CHIP_TYPE                xxxx    //flash:4MB,QFN40
#define BOARD_PORT_FILE             "CI-XXXX.c"
#endif

//使用IIC引脚配置
#define USE_IIC_PAD                     0           //1-配置  0-不配置 是否配置IIC引脚
#if (USE_BOARD == 4)
#define USE_IIC_PAD                     1           //双模拟麦+外部codec的板级必须使能IIC
#endif

//使用PDM配置
#define AUDIO_IN_FROM_DMIC              0           //1-数字  0-模拟    是否使用数字MIC输入音频

#define MIC_RECORD_IIS_SELECT           0      //MIC输入的IIS选择:0-内部codec IIS1   1-外部IIS0;
#define MIC_DIFF_SINGLE                 0      //1-单端  0-差分（通用模块都是差分模式，省成本的模块为单端(MICN_L 接GND)时，需要配置为SINGLE)
#define REF_DIFF_SINGLE                 1      //1-单端  0-差分（D类功放是采用单mic aec 使用 差分 mic ; AB类功放是采用单mic aec 使用 单端 mic)

//**通信串口引脚开漏模式使能配置
//注:推挽模式的IO只能对接3.3V电平的IO，开漏模式可以对接5V电平的IO(外部需要上拉到5V)
#define UART0_PAD_OPENDRAIN_MODE_EN     0   //0,UART0为推挽模式。1,UART0为开漏模式。
#define UART1_PAD_OPENDRAIN_MODE_EN     0   //0,UART1为推挽模式。1,UART1为开漏模式。
#define UART2_PAD_OPENDRAIN_MODE_EN     0   //0,UART2为推挽模式。1,UART2为开漏模式。

/***********************************1.2 时钟/内外置1.1V电压/自适应波特率 配置************************************/ 
//**时钟源配置
#define USE_EXTERNAL_CRYSTAL_OSC        1           //0-使用内部RC作为时钟源。1-使用外部晶振作为时钟源。
//内外置1.1V配置
#define USE_INNER_LDO3                  1           //0-外置1.1V 1-内置(外置DCDC会省功耗，成本会增高; 内置LDO功耗会更大一些)

//**波特率自适应功能配置
#if (USE_EXTERNAL_CRYSTAL_OSC == 0)                 //使用内部RC时,建议开启波特率自适应(需要电控增加对应支持)。
#define UART_BAUDRATE_CALIBRATE         1           //是否使能波特率自适应功能。
#define BAUDRATE_SYNC_PERIOD            300000      // 波特率同步周期，单位毫秒。
#define BAUDRATE_FAST_SYNC_PERIOD       5000        // 一次波特率同步失败后，下一次同步间隔，单位毫秒。
#define BAUD_CALIBRATE_MAX_WAIT_TIME    400         // 等待反馈包的超时时间，单位毫秒。
#endif

/***********************************1.3 双麦克时AEC的参考通路配置************************************/ 
/*1、双mic + aec算法必须外部挂codec作为信号回采(推荐7243e)
  2、双dmic + 模拟右麦/外部挂codec(推荐7243e)作为信号回采
  3、外部IIS0双麦 + 模拟右麦作为信号回采
*/
#if USE_AEC_MODULE && OFFLINE_DUAL_MIC_ALG_SUPPORT
#if AUDIO_IN_FROM_DMIC 
#define REF_IN_FROM_INNER_CODEC         1      //采用2 DMIC + 内部codec右通道作为信号回采
#define IF_USE_ANOTHER_CODEC_TO_GET_REF 0      //采用2 DMIC + 外挂codec输入REF信号
#elif MIC_RECORD_IIS_SELECT
#define REF_IN_FROM_INNER_CODEC         1      //采用外部IIS0双麦 + 模拟右麦作为信号回采
#else
#define IF_USE_ANOTHER_CODEC_TO_GET_REF 1
#endif
#endif

/***********************************1.4内置CODEC配置************************************/ 
//前级增益即MIC模拟增益，其实也是一种程控增益，但是相对于PGA，前级增益的步进更粗，范围更窄，它的增益可以设置为:0\6\9\12\20dB
#define CODEC_ADC_MIC_AMP_L                 INNER_CODEC_MIC_AMP_12dB        //左通道
#define CODEC_ADC_MIC_AMP_R                 INNER_CODEC_MIC_AMP_12dB        //右通道

//PGA是可编程增益放大器，PGA的增益设置有两种工作模式，固定增益模式和ALC自动增益调节模式
#define PGA_ALC_AUTO_MODE_ENABLE            1              //PGA ALC自动增益模式使能  0-关闭 1-使能(双麦算法时默认关闭ALC，该宏无效)
//注意：以下配置均为固定增益模式下的增益，其范围为-18dB到28.5dB，步进为1.5dB(双麦算法需保证左右通道增益一致)
#define PGA_GAIN_L                          20.0f         //正常工作下的左通道增益值,以及双麦算法时的左通道增益
#if    !OFFLINE_DUAL_MIC_ALG_SUPPORT || REF_IN_FROM_INNER_CODEC
#define PGA_GAIN_R                          4.0f          //正常工作下的右通道增益值,以及双麦算法时的右通道增益
#else
#define PGA_GAIN_R                          PGA_GAIN_L    //正常工作下的右通道增益值,以及双麦算法时的右通道增益
#endif
//注意：AEC工作时会关闭ALC,使用下面左右mic增益固定参数，其范围为-18dB到28.5dB，步进为1.5dB \
  IF_USE_ANOTHER_CODEC_TO_GET_REF使能时，左右通道为MIC都会设置到AEC_PGA_GAIN_L \
  IF_USE_ANOTHER_CODEC_TO_GET_REF未使能时, 左通道为MIC会固定设置到AEC_PGA_GAIN_L，右通道为REF会固定设置到AEC_PGA_GAIN_R
#define AEC_PGA_GAIN_L                      20.0f         //AEC处理时（以及以及AEC自动增益获取工具测试的初值）的左通道增益值
#if    !OFFLINE_DUAL_MIC_ALG_SUPPORT || REF_IN_FROM_INNER_CODEC
#define AEC_PGA_GAIN_R                      4.0f          //AEC处理时（以及以及AEC自动增益获取工具测试的初值）的右通道增益值
#else
#define AEC_PGA_GAIN_R                      AEC_PGA_GAIN_L//AEC处理时（以及以及AEC自动增益获取工具测试的初值）的右通道增益值
#endif
//数字增益的可调范围非常宽，digt_gain寄存器值设置为195的时候，增益为0dB，设置的寄存器值每增减1，数字增益就每增减0.5dB。数字增益的范围也就是-97dB到30dB。
#if (MIC_DIFF_SINGLE == 0)
#define DIG_GAIN_L                          1.0f
#else
#define DIG_GAIN_L                          6.0f 
#endif
#if (IF_USE_ANOTHER_CODEC_TO_GET_REF)//使用外部codec时，右通道使用MIC的差分单端判断
#if (MIC_DIFF_SINGLE == 0)
#define DIG_GAIN_R                          1.0f
#else
#define DIG_GAIN_R                          6.0f
#endif
#else
#if (REF_DIFF_SINGLE == 0)
#define DIG_GAIN_R                          1.0f
#else 
#define DIG_GAIN_R                          6.0f
#endif
#endif

/***********************************1.5 外置CODEC配置************************************/ 
#if (IF_USE_ANOTHER_CODEC_TO_GET_REF)
#define OUTSIDE_CODEC_GAIN                  14.0f   //AEC的参考回路使用外部CODEC时的增益值(默认是7243e)，范围为-95.5f到32.0f对应-95.5~+32dB
#endif

/***********************************1.6 IIS配置************************************/ 
#define INNER_CODEC_AUDIO_IN_USE_RESAMPLE   1   //默认打开   0:不开重采样   1：打开重采样，消耗24KB内存，识别效果会提升5个点左右
//IIS0用户可配,IIS1 IIS2不支持用户配置(目前RX、TX只支持同一种模式,并且采样率也需要一致；以下时钟相关配置同时使用在RX和TX)-
#define IIS0_MODE                           IIS_MASTER                    //IIS0 工作模式选择主或从
#define IIS0_OVER_SAMPLE                    IIS_MCLK_FS_256               //IIS0 过采样率，可配置为128/192/256/384（采样率为16khz，该比例为64，则mclk为4Mhz）
#define IIS0_CLK_SOURCE                     AUDIO_PLAY_CLK_SOURCE_IPCORE  //IIS0 时钟源,可选择为IPCORE/EXT_OSC/INTERNAL RC/PAD_IN
#define IIS0_MCLK_EN                        IIS_MCLK_OUT                  //IIS0 mclk是否从PAD输出,或者由外部输入
#define IIS0_DATA_FORMAT                    IIS_DF_IIS                    //IIS0 数据对齐方式，可配置为IIS格式标准/左对齐(MSB)格式标准/右对齐(LSB)格式标准
#define IIS0_SCK_LRCK_RATIO                 IIS_SCK_LRCK_64               //IIS0 sck与采样率的比值,可配置为32/64（采样率为16khz，该比例为64，则sck为1Mhz）
#define IIS0_TX_CHA                         IIS_TX_CHANNAL_TX0            //IIS0 TX 发送通道选择
#define IIS0_RX_CHA                         IIS_RX_CHANNAL_RX0            //IIS0 RX 接收通道选择
#define IIS0_OUTSIDE_MCLK_FRE               0                             //IIS0 外部送入的MCLK的时钟频率，只在使用外部MCLK做IIS时钟源时有效
#define IIS0_SCK_LRCK_OUT_EN                IIS_SCKLRCK_OUT               //IIS0 SCK/LRCK是否从PAD输出,或者由外部输入

#define IIS0_SAMPLE_RATE                    16000                         //IIS0 采样率，单位为khz，可配置为8k/16k/32k/48khz，软件内部音频处理为16khz采样率，若非16khz配合ci_alg_common_api.c中升降采样接口使用
#define IIS0_TX_CHANNEL_FLAG                3                             //IIS0 TX通道配置bit0 = 1选择左声道，bit1=1选择右声道，可以用或运算组合，3及为左右声道同时使能
#define IIS0_RX_CHANNEL_FLAG                3                             //IIS0 RX通道配置bit0 = 1选择左声道，bit1=1选择右声道，可以用或运算组合，3及为左右声道同时使能(作参考回路只需右声道)
#define IIS0_SAMPLE_DEPTH                   IIS_DW_16BIT                  //IIS0 采样位宽，可配置为16/20/24/32
#if IF_USE_ANOTHER_CODEC_TO_GET_REF
#if INNER_CODEC_AUDIO_IN_USE_RESAMPLE 
#define IIS0_SAMPLE_RATE                    32000                         //IIS0 采样率，单位为khz,使用重采样后外部codec输入的ref信号采样率也必须是32khz
#endif
#define IIS0_RX_CHANNEL_FLAG                2                             //IIS0 RX通道配置bit0 = 1选择左声道，bit1=1选择右声道，可以用或运算组合，3及为左右声道同时使能(作参考回路只需右声道)
#endif

/**********************************************2.算法参数配置*************************************************/  

/***********************************2.1 前端算法参数配置************************************/ 

/***********************2.1.1 GCC DOA参数配置****************************/ 
/* GCC DOA参数配置(注意：GCC DOA适用于声源到mic距离0.5m~3m的场景，GCC DOA不需要使用NN模型；
 * 当前参数适应15度内的俯仰角，大于15度俯仰角，需要根据GCC_DOA_MIC_DISTANCE_SCALE参数需要根据GCC_DOA_MIC_DISTANCE_SCALE说明来调)
*/  
#define GCC_DOA_MIC_DISTANCE              DISTANCE_40                         //mic间距(mm)
#define GCC_DOA_ENG_MIN_THR               0.2f                                //能量阈值,范围0~1，建议步长为0.1, 算法起效的最小能量阈值
#define GCC_DOA_DENOISE_EN                false                               //降噪开启效果更好，多耗内存6KB
#define GCC_DOA_DISTANCE_OFFSET_SCALE     0.00f                               //导向向量补偿因子，偏移值
//导向向量补偿因子,由基准值+偏移值组成；基准值为对应的mic间距定值，当角度往两端偏移时，比如声源真实角度为45°/135°，输出却为22°/157°时\
 ，需将偏移值调大，反之往中间偏移（输出为67°/112°）时，将偏移值调小，步进为0.01
#define GCC_DOA_MIC_DISTANCE_SCALE        DISTANCE_40_BASE_SCALE + GCC_DOA_DISTANCE_OFFSET_SCALE
#define GCC_DOA_OUT_TYPE                  1                                   //doa输出类型：0-周期输出角度  1-唤醒词和命令词都输出角度

/**********************2.1.2 AEC参数配置****************************/ 
#define AEC_CTRL_MODE                     ENABLE_PLAYING_STATE_MODE          //aec控制模式 ENABLE_PLAYING_STATE_MODE-根据播报状态控制AEC工作状态  \
                                                                                          COMPUTE_REF_AMPL_MODE-根据幅值控制AEC工作状态
#define AEC_REF_ENG_MIN_THR	              2000.0f                            //参考信号最小门限阈值，低于该门限阈值，AEC不进行计算      
#define AEC_TOP_CLIP_DETECT_ENABLE        1                                  //1-开启AEC削顶检查 0-关闭AEC削顶检查
#define AEC_TOP_CLIP_REDUCE_GAIN          2                                  //开启AEC_TOP_CLIP_DETECT_ENABLE的情况下，AEC处理期间，出现mic削顶后，减小的mic增益，保护AEC计算\
                                                                             设置范围推荐（0~4），该值设置过大会导致远场拾音效果下降，太小可能依旧会削顶\
                                                                            （减去增益后继续判断是否削顶，削顶则再减去该增益，最多减去3次AEC_TOP_CLIP_REDUCE_GAIN，该次播报结束后增益会返回到减增益之前的初值）
#define AEC_WIENER_RATIO                  1.0f                               //控制维纳降噪系数，1.0为默认参数,范围为0.1~1.9,步进0.1,\
                                                                             加大则人声保留更完整,但残留回声会相应变大；减小则回声消除更彻底，人声可能有轻微损失
#if USE_AEC_MODULE
#define AEC_INTERRUPT_TYPE                2                                  //默认2: 命令词和唤醒词都可打断  1: 只有命令词能打断   0:只有唤醒词能打断
#endif

/**********************2.1.3 NN VAD参数配置****************************/ 
#define NN_VAD_SENSITIVITY                                1              //灵敏度等级，设置0,1,2，对应低，中，高。灵敏度等级越高越灵敏;
#define NN_VAD_FRONT_PORCH                                25             //前窗口大小，前端回退25帧400ms,每帧16ms（不可超过PCM_MSG_STREAM_NUM）
#define NN_VAD_VALID_CHECK_MODE                           1              //VAD语音有效性检查模式：1-前置检查，等待判断语音有效后，再回退语音上传 2-后置检查，优先实时上传语音，检查有效性结果后通知WIFI端,建议关闭云端响应超时功能使能
#define NN_VAD_VALID_NUM                                  25             //VAD_ON 状态的至少满足的有效帧数,取值为18~35，每帧10ms，大于该阈值则视为语音有效，低于该阈值识别结果无效
#define NN_VAD_BACK_PORCH                                 30             //后窗口大小， VAD_ON->VAD_END 最多持续的帧数 ；取值20~40，每帧10ms（过小会导致说话停顿就VAD_END了)
//超时强制VAD END检测
#define NN_VAD_TIMEOUT_CHECK_ENABLE                       1              //0-不进行超时检测 1-进行超时检测-不可修改
#define NN_VAD_TIMEOUT_VALUE                              5              //NN_VAD_TIMEOUT_CHECK_ENABLE开启时，强制结束录音的时间(单位S)，上传音频 
#define NN_VAD_MIN_SPEECH_DB                              39             //最小人声能量,根据mic增益调整,推荐范围35db~50db（对应真实人声40~45db），越小支持的拾音越远；\
													                                               该数值和mic 灵敏度和codec的增益配置相关，mic灵敏度越大，codec增益配置大，该数值需要调高一点；反之则调低一点 

/**********************2.1.4 NN降噪参数配置****************************/ 
#define DEFAULT_DENOISE_BETA                              1.0f           //缺省降噪强度系数，范围（0.0 < beta <= 1.0），1.0为默认降噪强度（完全的降噪效果），beta越小降噪强度越低
#define DENOISE_STRENGTH_ADAPT_ENABLE                     0              //1-降噪强度自适应调整模式0-关闭降噪强度自适应，使用缺省降噪强度系数DEFAULT_DENOISE_BETA
#define DENOISE_STRENGTH_ADAPT_DEBUG_DB_EN                0              //在DENOISE_STRENGTH_ADAPT_ENABLE使能时，噪声估计调试打印使能，在降噪算法自适应beta模式下\
                                                                          首先调试打印判断需要开启beta自适应的噪声能量最小DB阈值，小于该阈值则使用默认降噪强度
/*
  降噪强度自适应模式参数
  以DENOISE_BETA_UPPER_LIMIT = 1.0f、DENOISE_BETA_LOWER_LIMIT = 0.9f\
  DENOISE_ENG_HIGHEST_THR = 90.0f 、DENOISE_ENG_LOWEST_THR = 75.0f为例
   
   * * * * * * * ** * * * * * * *
   * DENOISE_ENG_HIGHEST_THR以上 *  ———— beta = DENOISE_BETA_LOWER_LIMIT(0.9)
   * * * * * * * ** * * * * * * *
   *         86.25~90DB         *   ———— beta = 0.900
   * * * * * * * ** * * * * * * *
   *         82.5~86.25DB       *   ———— beta = 0.933
   * * * * * * * ** * * * * * * *
   *         78.75~82.5DB       *   ———— beta = 0.966
   * * * * * * * ** * * * * * * *
   *         75~78.75DB         *   ———— beta = DENOISE_BETA_UPPER_LIMIT(1.0f)
   * * * * * * * ** * * * * * * *
   * DENOISE_ENG_LOWEST_THR以下  *  ———— beta = 1.0f
   * * * * * * * ** * * * * * * *
*/                                                                          
#define DENOISE_BETA_UPPER_LIMIT                          1.0f           //beta上限值
#define DENOISE_BETA_LOWER_LIMIT                          0.9f           //beta下限值,推荐大于0.9
#define DENOISE_ENG_HIGHEST_THR                           90.0f          //噪声能量最高DB阈值
#define DENOISE_ENG_LOWEST_THR                            75.0f          //噪声能量最小DB阈值

/***********************************2.2 识别算法参数配置************************************/ 
/**********************2.2.1 解码参数配置****************************/ 
#define DECODER_MIN_ACTIVE                  (400)              //默认400，若节点小于8000，可尝试修改到300；最小值200，最大值800;值越大，内存消耗越多，识别速度越慢
#define DECODER_BEAM                        (9.5f)             //范围3.5~15.5;值越大,内存消耗越多,识别速度越慢
#define DEFAULT_CONFIDENCE                  (60)               // 配置默认置信度宏 :Excel未配置置信度的词条的默认值, 理论范围[1-255],建议范围[30-150] 
#define DEFAULT_CNT                         (6)                // 默认可信次数 :Excel未配置cnt的词条的默认值,理论范围[1-255],建议范围[4-20]
#define RECALL_RESULT_ENABLE                (1)                //语音识别结果回收使能配置（整段语音结束，无有效结果，再回溯搜索打分满足阈值[CNT不满足]的结果）,\
                                                                 开启人声噪音识别率会增加，但集外误识别会提升
#define RECALL_MIN_CNT                       2                 //RECALL_RESULT_ENABLE为1时，该参数配置有效，最小再召回可信次数 2-默认值 \
                                                                1-会提升4个点的识别率,但误识会增加 3-识别率最低,误识最低
#define RECALL_RESULT_MODE                  (0)                // RECALL_RESULT_ENABLE 为1 有效，语音识别结果回收使能作用模式，0 命令词和唤醒词; 1 唤醒词
#define RECALL_RESULT_MAX_FRM               (0)                // RECALL_RESULT_ENABLE 为1 有效，回收使能 在CNT基础上再增加超时帧数 , 10=300 ms ,理论范围 [0-200],建议范围[0-20]

//以下宏和响应速度相关
#define ADAPTIVE_CNT_ENABLE                 (1)                 //配置自适应cnt使能宏 ，1：打开自适应 cnt; 0：关闭，加快响应速度，但必须手动在excel配置短词的cnt；
#define MAX_STOP_CFD_ENABLE                 (0)                 // 打分超过阈值提前结束功能 0 关闭 1 开启(可提升响应速度),如果有长短词混淆，需要关闭该参数
#define MAX_STOP_CFD_WITHOUT_CNT            (50)                // MAX_STOP_CFD_ENABLE为1有效 无cnt配置的词，最大结束阈值,理论范围 [5-200],建议范围[50-150]； 
#define MAX_STOP_CFD_WITH_CNT               (60)                // MAX_STOP_CFD_ENABLE为1有效 有cnt配置的词，最大结束阈值,至少要比 MAX_STOP_CFD_WITHOUT_CNT 大,\
                                                                理论范围 [5-200],建议范围[50-150]；
#define DEFAULT_STOP_SILPROB                (0.9f)              // 识别结束停顿静音概率阈值,理论范围[0-1]，值越大支持的停顿时间越长
#define DEFAULT_STOP_SILCNT                 (1)                 // 识别结束静音连续次数宏：识别结束停顿静音,值越大支持的停顿时间越长,理论范围 [1-200] 建议范围 [1-10] \
                                                                   命令词模式设置为1，自然说模式设置为2


/**********************2.2.2 本地VAD参数配置****************************/ 
#define MAX_STOP_VAD_FRM                    (30 )                //配置VAD_END帧数宏 最大停顿静音间隔帧数 30=300ms 值越大识别响应时间会加长;过短导致出现截断，\
                                                                  长词识别为短词 ,建议：英文/小语种 该值至少35,理论范围 [1-60],建议范围[20-50]
#define VAD_SENSITIVITY                     VAD_SENSITIVITY_HIGH //用于配置VAD灵敏度，分高(默认)、中、低；该值配为低，在低信噪比情况下会降低识别率
#define VAD_START_BACK_FRAMES               (55)                 //VAD-START的时候，计算ASR特征回退的帧数,默认 55 帧 ，建议范围 20~60 ，太小 响应速度略快，\
                                                                  但极低音量、低信噪\比场景识别率有 3%~5% 左右下降。
#define VAD_START_BACK_FRAMES_ADAPTIVE_EN   (0)                  //VAD-START的时候，自适应配置计算ASR特征回退的帧数, \
                                                                 当使能后 通常情况为 VAD_START_BACK_FRAMES_FASTER 帧,当特殊情况配回 VAD_START_BACK_FRAMES 帧\
                                                                 当使用 PRO7 并非常在意响应速度(极低音量、极低信噪比识别会3%~5% 左右下降)，可以开启(开启 响应速度提升40~100ms)。
#define VAD_START_BACK_FRAMES_FASTER        (30)                 // 当 VAD_START_BACK_FRAMES_ADAPTIVE_EN 为 1有效 , VAD-START的时候，计算ASR特征回退的帧数\
                                                                 通常情况为 VAD_START_BACK_FRAMES_FASTER 帧，特殊情况自动配回 VAD_START_BACK_FRAMES

/**********************2.2.3 自学习参数配置****************************/ 
//**自学习功能-请在安静环境下，用清晰洪亮的声音进行指令学习，避免环境噪音过大和学习者声音过小导致学习不成功 
//**注意：在线SDK只支持唤醒词学习，为了避免指令词被学习成模版，请确保cmd_info.xls中词条语义ID、命令词ID与需学习的词条语义ID、命令词ID不重复                                    
#if USE_CWSL
#define CWSL_WAKEUP_NUMBER          2           // 可学习的唤醒词数量-最大支持2个
#define WAKE_UP_ID                  1           // 学习的唤醒词对应的命令词ID
#define CWSL_REG_TIMES              1           // 学习时 每个词需说几遍，默认 1 遍即可,支持1、2遍,FOR_REG_2TIMES_FLOW_V2 配置 1时,最大支持 3 遍;
#define CWSL_WAKEUP_THRESHOLD       37          // 学习的唤醒词阈值门限，越小越灵敏，默认 37, 最小可配置到 32;
#define CWSL_CMD_THRESHOLD          35          // 学习的命令词阈值门限，越小越灵敏，默认 35，最小可配置到 30；
#define FOR_REG_2TIMES_FLOW_V2      0           // 学习时，说两遍/三遍逻辑，版本二流程，后续均和第一次的比较，一致学习成功，不一致，最多支持说 3 次\
                                                     FOR_REG_2TIMES_FLOW_V2 配置 1时, CWSL_REG_TIMES 必须是 2或3	
#define CWSL_REG_VAD_LEVEL          0           // 学习过程，灵敏度选项配置： 0 低灵敏度，可减少噪声对学习的干扰，需学习过程大声说话；1 高灵敏度，\
                                                但也可以导致干扰噪声干扰学习
#define CICWSL_TOTAL_TEMPLATE       CWSL_WAKEUP_NUMBER*3           //可存储模板数量

#if (CWSL_REG_TIMES == 3)
#define FOR_REG_2TIMES_FLOW_V2      1
#elif (CWSL_REG_TIMES > 3)
#error "The CWSL_REG_TIMES max 3\n"
#endif
#endif

/**********************************************3.应用参数配置*************************************************/  
/**********************3.1 识别中间结果打印使能****************************/ 
#define ASR_MIDDLE_RESULT_OUT_EN                 0               //输出识别中间结果使能
/**********************3.2 日志串口配置****************************/ 
#define CONFIG_CI_LOG_UART                       HAL_UART0_BASE  //配置log输出使用的串口，请勿与protocol共用同一个串口
/**********************3.3 串口采音配置****************************/ 
/*  上传双/四声道数据;(注意：确保bnpu端和host端的对应串口需要关闭，打开PC的串口录音软件，再开机，避免数据错帧导致录音异常)
    2通道不压缩时多消耗12KB SYS内存，4通道G722压缩多消耗9KB ASR内存和17KB SYS内存,内存不够可先关闭重采样，
 */
#define DEBUG_AUDIO_UART_UPLOAD_EN                0 //串口采音使能
#if DEBUG_AUDIO_UART_UPLOAD_EN
#define DEBUG_AUDIO_4CH_UART_UPLOAD_EN            0 //1 ：4路音频采音(MICL/MICR/REF/DST1)经串口输出，需配合 DEBUG_AUDIO_4CH_G722_EN=1；\
                                                      0 : 2路音频采音不压缩，可采音板实时采音，需 2M 波特率；
#define USE_UART_SEND_PRE_RSLT_AUDIO_NUMBER       HAL_UART2_BASE          //用哪个UART口将算法处理后的音频数据送出
#define USE_UART_SEND_PRE_RSLT_AUDIO_BAUD         UART_BaudRate2M     //串口采音波特率,两路实时采音不进行G722压缩时必须使用2M波特率
#define USE_UART_SEND_PRE_RSLT_AUDIO_BUF_LEN      1024*10                 //串口采音BUF大小
#if DEBUG_AUDIO_4CH_UART_UPLOAD_EN
/*不压缩时需要2Mbps波特率，串口传输时存在丢字节导致输出解析错位 */
#define DEBUG_AUDIO_4CH_G722_EN                1 
#endif
#endif
/**********************3.4 语音上传和云端数据下行播放参数配置****************************/ 
#if !AUDIO_DATA_UPLOAD_BY_UART && !AUDIO_DATA_PLAY_BY_UART
#define WIFI_CMD_BY_UART                                (1)            //通过串口和wifi指令交互配置
#endif

#if NET_AUDIO_PLAY_BY_PCM || NET_AUDIO_PLAY_BY_G722
#define NET_AUDIO_PCM_G722_SAMPLE_RATE                    16000         //云端播放pcm/g722格式的采样率,可配置为8k/12k/16k/24k/32k/44.1k/48khz，
#define NET_AUDIO_PCM_G722_CHANNEL_FLAG                   1             //（默认单声道播放）云端播放pcm/g722格式的声道通道配置bit0 = 1选择左声道，bit1=1选择右声道，可以用或运算组合，3及为左右声道同时使能
#define NET_AUDIO_PCM_G722_SAMPLE_DEPTH                   16            //云端播放pcm/g722格式的采样位宽，可配置为16/20/24/32
#define NET_AUDIO_PCM_FRAME_SIZE                          (512)         //云端播放pcm格式的每帧size（16khz 16ms数据为512个字节）
#define NET_AUDIO_G722_FRAME_SIZE                         (320)         //云端播放g722格式的每帧size（16khz 40ms 为320个字节）
#endif
//网络数据播放高码率或者高采样率时,播放卡顿需要适当增加 ：1、解码buf内存SAP_SOURCE_DATA_BUF_SIZE 2、每帧请求数据量REQUEST_ONE_FRAME_SEZIE 3、网络播放流缓存大小NET_AUDIO_PLAY_STREAM_SIZE
//高采样率的PCM数据需要加大串口波特率
#if NET_AUDIO_PLAY_BY_G722
#define SAP_SOURCE_DATA_BUF_SIZE                          160
#elif NET_AUDIO_PLAY_BY_PCM
#define SAP_SOURCE_DATA_BUF_SIZE                          1024*2         //1024*2适用16khz采样率
#elif NET_AUDIO_PLAY_BY_MP3
#define SAP_SOURCE_DATA_BUF_SIZE                          1024           //比特率大于128kbps，建议使用2048
#else
#define SAP_SOURCE_DATA_BUF_SIZE                          160
#endif

#if NET_AUDIO_PLAY_BY_PCM
#define REQUEST_ONE_FRAME_SEZIE                           1024*4          //pcm单帧请求播放数据大小最小为4K，不然会出现播放卡顿，采样率增加需相应增加
#elif   NET_AUDIO_PLAY_BY_MP3  
#define REQUEST_ONE_FRAME_SEZIE                           1024*1          //请求单帧播放数据大小(mp3)，比特率大于128kbps，建议使用1024*2
#elif   NET_AUDIO_PLAY_BY_OPUS  
#define REQUEST_ONE_FRAME_SEZIE                           320             //请求单帧播放数据大小(opus) 4帧*1280
#elif   NET_AUDIO_PLAY_BY_G722  
#define REQUEST_ONE_FRAME_SEZIE                           1024*1          //请求单帧播放数据大小(opus) 4帧*1280
#else
#define REQUEST_ONE_FRAME_SEZIE                           128             //不播放音频文件，缓存可以减小
#endif

#if NET_AUDIO_PLAY_BY_G722
#define NET_AUDIO_PLAY_STREAM_SIZE                        1024*8
#elif NET_AUDIO_PLAY_BY_PCM
#define NET_AUDIO_PLAY_STREAM_SIZE                        1024*8          //1024*8适用16khz采样率
#elif NET_AUDIO_PLAY_BY_MP3
#define NET_AUDIO_PLAY_STREAM_SIZE                        1024*4          //比特率大于128kbps，建议使用1024*8
#else
#define NET_AUDIO_PLAY_STREAM_SIZE                        0
#endif

//语音数据上传是否带协议头
#define AUDIO_SEND_WITH_PROTOCOL_HEADER                   1            //1-带协议头上传(默认) 0-不带协议直接传裸数据-调试使用

//云端交互相关配置          
#define CUR_INTERACTION_MULTI_ROUND_ENABLE                1              //1-多轮 0-单轮
#define UPLOAD_PLAY_FULL_DUPLEX_ENABLE                    1              //全双工处理,播放音频的同时支持vad检测音频上传
#define VAD_START_STOP_PLAY_ENABLE                        0              //全双工模式下，vad起来，立刻停止播放 1-停止播放 0-不停止播放
#define CLOUD_ANS_TIME_OUT_ENEABLE                        0              //云端响应超时功能使能
#define CLOUD_ANS_TIME_OUT_VALUE                          6              //云端响应超时时间6S，必须大于0
#define AUDIO_PLAY_MODE                                   1              //1-支持打断当前播放 0-不支持，顺序播放-暂时不用
#define UPLOAD_NNDENOISE_AUDIO_DATA_ENABLE                1              //1-上传降噪的音频 0-上传非降噪的音频
#define CHECK_NET_WORK_STATE_ENABLE                       0              //检测网络状态后再上传音频宏
#define PCM_MSG_STREAM_NUM                                30             //原始数据stream buf大小   //30
#define LOCAL_REC_VAD_END_ENABLE                          1              //本地识别后立即停止VAD,若不停止，在半双工并且本地词条有播报情况下,播报时会丢录音
#define PCM_ALG_FRAME_LEN                                 512            //前端原始音频16K采样，每帧256个点共512字节

#if NET_AUDIO_PLAY_BY_PCM || NET_AUDIO_PLAY_BY_G722
#if SIMPLE_AUDIO_PLAYER_ENABLE
#define NET_PLAY_PCM_OR_G722_ENABLE_LOCAL_PLAY            1             //云端播放通过pcm、G722格式时,同时开启本地播报
#endif
#define USE_HP_OUT_NET_AUDIO                              1             //使用hpout直接输出云端音频
#define PLAY_PCM_FRAME_BUF_LEN                         1024*6           //pcm播放缓冲大小 
#endif

#if (AUDIO_DATA_PLAY_BY_UART)
#define AUDIO_PLAY_USE_OUTSIDE                          (1)              //启用自定义外部数据源播放
#define AIOT_AUDIO_PLAY_BUF_SIZE                        (6 * 1024)       //播放器缓存大小
#define PLAY_BUF_GET_DATA_MIN_SIZE                      (4 * 1024)       //可播放数据小于此大小开始从网络获取下一帧数据--最小4K
#endif

#if AUDIO_COMPRESS_RECORD_DISABLE
#define PCM_UPLOAD_QUEUE_NUM                              60             //上传数据队列大小   //60
#define COMPRESS_NEED_PCM_LEN                             0              //不需要压缩
#define COMPRESS_UPLOAD_QUEUE_TIEM_SIZE                   512             //上传队列元素大小,speex编码后每帧长度42字节
#elif AUDIO_COMPRESS_SPEEX_ENABLE
#define PCM_UPLOAD_QUEUE_NUM                              60             //上传数据队列大小   //60
#define COMPRESS_NEED_PCM_LEN                            320             //不可修改(320个点 short)
#define COMPRESS_UPLOAD_QUEUE_TIEM_SIZE                   43             //上传队列元素大小,speex编码后每帧长度42字节
#elif AUDIO_COMPRESS_OPUS_ENABLE         
#define USE_ALLOCA                                        1              //不可修改
#define VAR_ARRAYS                                        1              //不可修改
#define OPUS_BUILD                                        1              //不可修改
#define DISABLE_FLOAT_API                                 1              //不可修改
#define FIXED_POINT                                       1              //不可修改
#define REMOVE_FOR_MALLOC                                 1              //不可修改
#define HAVE_CONFIG_H                                     1              //不可修改          
//节约内存和代码体积新增的配置
#define OPUS_SILK_SUPPORT                                 0              //不可修改    
#define OPUS_MUILT_FRAME                                  1              //不可修改    
#define PCM_UPLOAD_QUEUE_NUM                              30             //上传数据队列大小   //60
#define COMPRESS_NEED_PCM_LEN                            640             //不可修改 (640个点 short)
#define COMPRESS_UPLOAD_QUEUE_TIEM_SIZE                   80             //上传队列元素大小,opus编码后每帧长度80字节
#elif AUDIO_COMPRESS_G722_ENABLE
#define PCM_UPLOAD_QUEUE_NUM                              60             //上传数据队列大小   //60
#define COMPRESS_NEED_PCM_LEN                            320             //不可修改(320个点 short)
#define COMPRESS_UPLOAD_QUEUE_TIEM_SIZE                  160             //上传队列元素大小,speex编码后每帧长度42字节
#else
#define COMPRESS_UPLOAD_QUEUE_TIEM_SIZE                  128             //上传队列元素大小,speex编码后每帧长度42字节
#endif

#if NET_AUDIO_PLAY_BY_MP3 && AUDIO_COMPRESS_RECORD_DISABLE && UPLOAD_PLAY_FULL_DUPLEX_ENABLE && !VAD_START_STOP_PLAY_ENABLE
#define NETWORK_RECV_BUFF_MAX_SIZE                       (REQUEST_ONE_FRAME_SEZIE*3 + 20) //接收网络端串口数据最大size-帧头+预留4字节
#define AIOT_AUDIO_PLAY_BUF_SIZE                         (12 * 1024)       //播放器缓存大小
#else
#define NETWORK_RECV_BUFF_MAX_SIZE                       (REQUEST_ONE_FRAME_SEZIE + 20) //接收网络端串口数据最大size-帧头+预留4字节
#endif
#if AUDIO_COMPRESS_RECORD_DISABLE
#define NETWORK_SEND_BUFF_MAX_SIZE                       (COMPRESS_UPLOAD_QUEUE_TIEM_SIZE/4 + 16)   //发送串口数据到网络端最大size,pcm数据分4包传输，避免丢数据
#elif AUDIO_COMPRESS_G722_ENABLE
#define NETWORK_SEND_BUFF_MAX_SIZE                       (200)         //发送串口数据到网络端最大size
#else 
#define NETWORK_SEND_BUFF_MAX_SIZE                       (COMPRESS_UPLOAD_QUEUE_TIEM_SIZE + 16)     //发送串口数据到网络端最大size
#endif
#if (NET_AUDIO_PLAY_BY_PCM) && AUDIO_COMPRESS_RECORD_DISABLE
#define NETWORK_SEND_BUFF_NUM                             10           //发送串口数据到网络端缓冲区个数  
#elif NET_AUDIO_PLAY_BY_MP3 && AUDIO_COMPRESS_RECORD_DISABLE && UPLOAD_PLAY_FULL_DUPLEX_ENABLE && !VAD_START_STOP_PLAY_ENABLE
#define NETWORK_SEND_BUFF_NUM                             8            //发送串口数据到网络端缓冲区个数 
#else
#define NETWORK_SEND_BUFF_NUM                             5            //发送串口数据到网络端缓冲区个数  
#endif

/**********************3.5 协议串口配置****************************/ 
//和WiFi通信串口及参数配置
#if AUDIO_DATA_PLAY_BY_UART || AUDIO_DATA_UPLOAD_BY_UART || WIFI_CMD_BY_UART
#define UART_NUM_SEND_PLAY_AUDIO_NUMBER                  HAL_UART1_BASE           //网络端交互的串口
#define UART_NUM_SEND_PLAY_AUDIO_BAUDRATE                UART_BaudRate921600      //网络端交互的串口波特率
#endif

//语音模块通讯协议（离在线主要使用与WIFI通信的串口UART_NUM_SEND_PLAY_AUDIO_NUMBER，根据需求增加该串口）
#define MSG_COM_USE_UART_EN            0   //0,关闭语音模块通讯协议。1,开启语音模块通讯协议。
#define UART_PROTOCOL_NUMBER           (HAL_UART2_BASE)    //语音模块协议使用的串口，请勿与log共用同一个串口。
#define UART_PROTOCOL_BAUDRATE         (UART_BaudRate921600) //语音模块协议使用的串口波特率。
#define UART_PROTOCOL_VER              2   //语音模块协议版本号:1,一代协议。2,二代协议，255,平台生成协议

#define CLOUD_UART_PROTOCOL_EN         0   //云端协议使能-只有在启英开发者平台做固件配协议能用
#if CLOUD_UART_PROTOCOL_EN
#define CLOUD_CFG_UART_SEND_EN         1   //使能串口发送数据
#define CLOUD_CFG_PLAY_EN              1   //播报音使能
#define CLOUD_CFG_UART_PORT	         ((UART_TypeDef*)(HAL_UART1_BASE))// HAL_UART0_BASE ~ HAL_UART2_BASE，请勿与log共用同一个串口
#define CLOUD_CFG_UART_BAUND_RATE    UART_BaudRate9600
#endif

/**********************3.6 OTA功能使能配置****************************/ 
// 开启OTA功能，需修改打包升级工具版本，并在firmware\config.ini文件中设置成firmware_version=FW_V4，升级与被升级固件均要FW_V4版本工具打包生成
#define CI_OTA_ENABLE                   0   // 1使能 0 不使能

/**********************3.7 AEC自动获取增益工具配置****************************/ 
//在安静环境下使用该工具，自动获取AEC处理时较好的MIC和REF增益以及AEC使能阈值，增益值和使能阈值会在输出日志并且发送指令到主端,该流程仅测试使用，\
根据SDK配置的mic和ref通路情况将AEC_PGA_GAIN_L、AEC_PGA_GAIN_R、OUTSIDE_CODEC_GAIN设置为测试得到的增益值，以及AEC_REF_ENG_MIN_THR设置为测试得到的使能门限值
#define AUTO_GET_AEC_GAIN               0 
#if AUTO_GET_AEC_GAIN
#define GET_AEC_GAIN_AUDIO_ID           3000  //AEC增益测试音频ID
#define GET_AEC_REF_THR_AUDIO_ID        3001  //AEC使能阈值（参考信号能量）测试音频ID
/*输入幅值上限，确保mic输入不截幅,根据应用时需播报的音频最大dBFS给对应阈值（dBFS是数字音频系统中描述信号幅度的单位）：
  0~-2dBFS，ENERGE_UP_THR阈值为4500
  -2~-6dBFS，ENERGE_UP_THR阈值为6000
  -6dBFS以下，ENERGE_UP_THR阈值为7000
  */
#define ENERGE_UP_THR                   7000
#define GET_AEC_GAIN_POEWR_ON_START     1      //1：上电后直接开启AEC增益测试，使用默认参数 0：通过指令开启AEC增益测试
#define DEFAULT_MIN_MIC_GAIN            12     //默认最小MIC增益，最终输出的MIC增益不会小于该值，范围为（-18~28）
#define DEFAULT_EXPECT_VOLUM            5      //默认期望的播放音量等级，最终输出的音量等级可能会减小，范围（1~7）
#endif

/**********************3.8 生产测试配置****************************/ 
#define IIS_CHANNEL_ENG_CALC_EANBLE                       0              //iis通道能量计算
#define ENG_CALC_INTERVAL_FRAME                           10             //iis通道能量10帧计算一次(可根据需求修改)
#define CIAS_HAVE_AUDIO_ENG_MICL                          50             //左MIC有音频能量阈值设置，默认50db    
#define CIAS_HAVE_AUDIO_ENG_MICR                          50             //右MIC有音频能量阈值设置，默认50db    
#define CIAS_HAVE_AUDIO_ENG_REFL                          50             //REFL有音频能量阈值设置，默认50db    
#define CIAS_HAVE_AUDIO_ENG_REFR                          50             //REFR有音频能量阈值设置，默认50db    
#define CIAS_UPLOD_FACTORY_TEST_REAL_VAL                  0              //上传音频上传过程中的实时值 0-不上传 1-上传  


/**********************3.9 播放器参数配置****************************/ 
#if  USE_HP_OUT_NET_AUDIO
#define AUDIO_PLAYER_ENABLE             0   //是否启用音频播放器。0:不启用,1:启用。不使用播放功能时，启用本地mp3播放器多占用内存：默认播放器52K、精简播放器36K
#else
#define AUDIO_PLAYER_ENABLE             1   //是否启用音频播放器。0:不启用,1:启用。不使用播放功能时，启用本地mp3播放器多占用内存：默认播放器52K、精简播放器36K
#endif
                                            //关闭此功能可以节省内存空间。
#if  NET_PLAY_PCM_OR_G722_ENABLE_LOCAL_PLAY
#define AUDIO_PLAYER_ENABLE             1   //是否启用音频播放器。0:不启用,1:启用。不使用播放功能时，启用本地mp3播放器多占用内存：默认播放器52K、精简播放器36K
#endif
#define PLAYER_CONTROL_PA               0   //是否有播放器控音频功放开关。0:功放常开,1:播放器在需要播放时才打开,但可能增加一点每一次播放的延迟时间
#define VOLUME_MAX                      7   //设置音量调节的上限值，对应硬件支持的最大音量。
#define VOLUME_MIN                      1   //设置音量调节的下限值，对应最小音量。
#define VOLUME_DEFAULT                  5   //设置音量调节的默认值。

#if AUDIO_PLAYER_ENABLE || NET_AUDIO_PLAY_BY_MP3
#if SIMPLE_AUDIO_PLAYER_ENABLE
#if NET_AUDIO_PLAY_BY_MP3
#define MP3_NO_CONSTANT_BITRATE         0   //云端播放MP3是否开启非恒定比特率的模式VBR、ABR适配
#define MP3_MAX_DECODE_ERR              6   //云端播放MP3错帧后跳帧次数，超过该次数后退出播放
#endif
#else
#define USE_MP3_DECODER                 1   //为1时加入mp3解码器，1:是 0:否。
#define AUDIO_PLAY_SUPPT_MP3_PROMPT     1   //播放器是否开启mp3提示音，1:是 0:否。
#define USE_PROMPT_DECODER              1   //播放器是否支持prompt解码器，1:是 0:否。
#endif
#endif

/**********************3.10 提示播报+退出唤醒时间配置****************************/ 
#define PLAY_WELCOME_EN                 1      //是否在启动时播放开机提示音。1:是 0:否。
#define PLAY_ENTER_WAKEUP_EN            1      //是否在唤醒时播放提示音。1:是 0:否。
#define PLAY_EXIT_WAKEUP_EN             1      //是否在切换到只监听唤词状态时播放提示音。1:是 0:否。
#define PLAY_OTHER_CMD_EN               1      //是否在识别到命令词时播放提示音。1:是 0:否。
#define EXIT_WAKEUP_TIME                30*1000   //退出唤醒超时时间,单位毫秒。超过此配置指定的时间长度内没有识别到任何命令词，就会切换到只监听唤词状态。

/**********************3.11 是否使用独立唤醒词模型配置****************************/ 
#define USE_SEPARATE_WAKEUP_EN                1       //是否使用独立的唤醒词模型。1:是 0:否。

#define DEFAULT_MODEL_GROUP_ID                1       //模型ID,用于指定上电启动时，默认进入的语言模型。通常0为命令词模型,1为唤醒词模型
    
#if (!USE_SEPARATE_WAKEUP_EN)    
#undef DEFAULT_MODEL_GROUP_ID    
#define DEFAULT_MODEL_GROUP_ID                 0
#endif

/**********************3.12 两份固件动态切换应用配置****************************/ 
#define USER_CODE_SWITCH_ENABLE     (0)                                 //两份code动态切换功能
#if USER_CODE_SWITCH_ENABLE
#define UART_PROTOCOL_NUMBER           (HAL_UART1_BASE)      
#define UART_PROTOCOL_BAUDRATE         (UART_BaudRate115200)    //TTS默认波特率115200
#define USER_CODE2_SAVE_ADDR           0x4000-8                 //code固件地址存放位置
#endif 

/**********************************************4.参数配置检查*************************************************/  

#if (CONFIG_CI_LOG_UART == USE_UART_SEND_PRE_RSLT_AUDIO_NUMBER)
#error  "log and debug audio uart number must not same!"
#endif

#if USE_BEAMFORMING_MODULE  || (USE_NN_DOA || USE_GCC_DOA) || USE_DEREVERB_MODULE || USE_DUAL_MIC_ANY
#if (USE_BOARD == 1 || USE_BOARD == 2)
 #error "use CI_D12GS01J board not support dual mic alg !\n"    //131x不支持双mic算法
#endif
#define HOST_CODEC_CHA_NUM  2
#define OFFLINE_DUAL_MIC_ALG_SUPPORT    1
#else
#define HOST_CODEC_CHA_NUM              1
#define OFFLINE_DUAL_MIC_ALG_SUPPORT    0
#endif

#if USE_AEC_MODULE
    #define IF_JUST_CLOSE_HPOUT_WHILE_NO_PLAY   1
    #define HOST_CODEC_CHA_NUM  2

    #if USE_SED_SNORE || USE_SED_CRY
    #error "aec + sed detection alg not support!\n"    //事件检测不支持AEC
    #endif
#endif


#if (USE_BOARD == 1 || USE_BOARD == 2)
#if USE_BEAMFORMING_MODULE  || (USE_NN_DOA || USE_GCC_DOA)|| USE_DEREVERB_MODULE || USE_DUAL_MIC_ANY
 #error "use CI_D12GS01J board not support aec and dual mic alg!\n"    //131x不支持aec和双 mic算法
#endif
#endif

#if (MIC_RECORD_IIS_SELECT&&IF_USE_ANOTHER_CODEC_TO_GET_REF)
#error "no support MIC_RECORD_IIS_SELECT && IF_USE_ANOTHER_CODEC_TO_GET_REF toghter\n"
#endif
#if REF_IN_FROM_INNER_CODEC && IF_USE_ANOTHER_CODEC_TO_GET_REF
#error "no support REF_IN_FROM_INNER_CODEC && IF_USE_ANOTHER_CODEC_TO_GET_REF toghter\n"
#endif

#if ((CI_CHIP_TYPE == 1302) && (MIC_RECORD_IIS_SELECT || USE_IIS1_OUT_PRE_RSLT_AUDIO\
    || IF_USE_ANOTHER_CODEC_TO_GET_REF) && AUDIO_IN_FROM_DMIC)
#error "1302, no support dmic && iis\n"
#endif

#if CI_OTA_ENABLE && !USE_EXTERNAL_CRYSTAL_OSC
 #error "enable ota USE_EXTERNAL_CRYSTAL_OSC must set 1\n"
#endif

#if USE_BEAMFORMING_MODULE && USE_PWK
 #error "bf + pwk algorithm, not support\n"
#endif
//双麦算法和AEC在不接外部codec做AEC信号回采，不能同时使用
#if OFFLINE_DUAL_MIC_ALG_SUPPORT  && USE_AEC_MODULE
  #if AUDIO_IN_FROM_DMIC
    #if !IF_USE_ANOTHER_CODEC_TO_GET_REF && !REF_IN_FROM_INNER_CODEC
    #error "doa + aec algorithm, requires external codec1\n"
    #endif
  #else
    #if !IF_USE_ANOTHER_CODEC_TO_GET_REF
    #error "doa + aec algorithm, requires external codec\n"
    #endif
  #endif
#endif

#if USE_ANY_MIC || USE_NN_DOA || USE_GCC_DOA
    #if HOST_CODEC_CHA_NUM != 2
    #error "algorithm requires tow mic, please set HOST_CODEC_CHA_NUM = 2\n"
    #endif
#endif

#if (AUDIO_DATA_UPLOAD_BY_UART&&(USE_PWK))                       //语音上传功能不能开USE_PWK
#error "ONLY USE_NULL WITH NO PWK SUPPORT AUDIO_DATA_UPLOAD_BY_UART!"
#endif


#if USE_ALC_AUTO_SWITCH_MODULE && ((USE_NN_DOA || USE_GCC_DOA) || USE_DEREVERB_MODULE || USE_BEAMFORMING_MODULE || USE_AEC_MODULE)
#error "two mic alg, no alc auto\n"
#endif

#if USE_AEC_MODULE && OFFLINE_DUAL_MIC_ALG_SUPPORT && !AUDIO_IN_FROM_DMIC && !MIC_RECORD_IIS_SELECT && (USE_BOARD != 4) 
#error "dual analog mic alg + aec , must use CI_D06GT01J board\n"
#endif

//注意开doa+aec算法或者开自学习，由于内存原因，需要关闭重采样，如果要使用doa+aec算法又要保证识别效果，请使用HPOUT、IIS输出方案
#if ((USE_NN_DOA&&USE_AEC_MODULE) \
 || ((USE_CWSL)&& (AUDIO_COMPRESS_SPEEX_ENABLE || AUDIO_COMPRESS_OPUS_ENABLE))\
 || (AUDIO_COMPRESS_OPUS_ENABLE && NET_AUDIO_PLAY_BY_MP3))
//  #error "cur combination use resample, RAM no enough\n"
#endif


//单麦就近唤醒 和 动态ALC和深度降噪,自学习，事件检测算法不能同时使用
#if USE_PWK && (USE_ALC_AUTO_SWITCH_MODULE || USE_NN_DENOISE || USE_CWSL || USE_SED_CRY || USE_SED_SNORE)
#error "can't be toghter pwk\n"
#endif

//语音模块协议 和 云端协议使能不能同时使用
#if MSG_COM_USE_UART_EN && CLOUD_UART_PROTOCOL_EN
#error "MSG_COM_USE_UART_EN and CLOUD_UART_PROTOCOL_EN, can't be toghter\n"
#endif

#if NET_AUDIO_PLAY_BY_OPUS || NET_AUDIO_PLAY_BY_G722
#if !SIMPLE_AUDIO_PLAYER_ENABLE
#error  "play opus or G722 audio data, must set makefile USE_AUDIO_PLAYER_TYPE = 0"
#endif
#endif
#if AUDIO_COMPRESS_SPEEX_ENABLE && AUDIO_COMPRESS_OPUS_ENABLE
#error  "audio compress not support opus and speex at the same time"
#elif AUDIO_COMPRESS_G722_ENABLE && AUDIO_COMPRESS_SPEEX_ENABLE
#error  "audio compress not support g722 and speex at the same time"
#elif AUDIO_COMPRESS_G722_ENABLE && AUDIO_COMPRESS_OPUS_ENABLE
#error  "audio compress not support g722 and opus at the same time"
#endif
#if NET_AUDIO_PLAY_BY_MP3 && NET_AUDIO_PLAY_BY_PCM
#error  "audio play not support mp3 and pcm at the same time"
#elif NET_AUDIO_PLAY_BY_MP3 && NET_AUDIO_PLAY_BY_OPUS
#error  "audio play not support mp3 and opus at the same time"
#elif NET_AUDIO_PLAY_BY_OPUS && NET_AUDIO_PLAY_BY_PCM
#error  "audio play not support opus and pcm at the same time"
#elif NET_AUDIO_PLAY_BY_G722 && NET_AUDIO_PLAY_BY_PCM
#error  "audio play not support g722 and pcm at the same time"
#elif NET_AUDIO_PLAY_BY_G722 && NET_AUDIO_PLAY_BY_OPUS
#error  "audio play not support g722 and opus at the same time"
#elif NET_AUDIO_PLAY_BY_G722 && NET_AUDIO_PLAY_BY_MP3
#error  "audio play not support g722 and mp3 at the same time"
#endif
//opus+mp3算法组合不支持自学习和双麦算法，内存不足
#if AUDIO_COMPRESS_OPUS_ENABLE && NET_AUDIO_PLAY_BY_MP3 && USE_CWSL
#error  "opus compress and mp3 play not support cwsl at the same time"
#endif
#if NET_AUDIO_PLAY_BY_OPUS && AUDIO_COMPRESS_OPUS_ENABLE
#error  "opus play and opus record not support at the same time"
#endif
#if (NET_AUDIO_PLAY_BY_MP3)&&(USE_NN_DOA || USE_GCC_DOA) && AUDIO_COMPRESS_OPUS_ENABLE
#error  "mp3 play and dual mic alg and opus compress not support at the same time"
#endif
#if (NET_PLAY_PCM_OR_G722_ENABLE_LOCAL_PLAY) && AUDIO_COMPRESS_OPUS_ENABLE
#error  "g722/mp3 play and local play and opus compress not support at the same time"
#endif
#if AUDIO_COMPRESS_SPEEX_ENABLE && NET_AUDIO_PLAY_BY_OPUS
#error  "speex compress and opus play not support at the same time"
#endif

#if USE_EXTERNAL_CRYSTAL_OSC != 1
#error  "aiot application must be USE_EXTERNAL_CRYSTAL_OSC  = 1"
#endif



#endif /* _USER_CONFIG_H_ */
