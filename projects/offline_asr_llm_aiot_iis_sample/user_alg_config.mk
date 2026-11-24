########工程私有算法功能配置(仅对本工程生效, 不再使用 system/user_alg_config.mk)########
# 编译本工程时, 会把本文件各字段的值与同目录 config.a(配置快照)比对:
# 只要有不一致, 就自动进入 components/bnpu_core_alg_pro/project_file 重建算法库,
# 并把新的 ci130x_asr_alg_bnpu.a 输出到本工程 app 目录
########单一算法使能定义(注意:值后更不能有空格)########
#只用ASR识别功能(注意：除了事件检测功能其他功能都依赖识别，都必须默认开启)
USE_ASR           := 1
#任意mic识别
USE_ANY_MIC       := 0
#使用回声消除模块:1-开启/0-关闭 会占用28K的空间 -单/双mic可用，130x内部有2个codec;如果同时使用双mic算法+加AEC算法，需要硬件再加一个codc作为AEC的信号回采
USE_AEC           := 1
#自学习功能-请在安静环境下，用清晰洪亮的声音进行指令学习，避免环境噪音过大和学习者声音过小导致学习不成功
USE_CWSL          := 0
#NN深度降噪    
USE_NN_DENOISE    := 1
#NN VAD
USE_NN_VAD        := 1
#NN DOA - 暂不支持
#USE_NN_DOA        := 0
#GCC DOA
USE_GCC_DOA       := 0

#IIS 开启IIS采音功能,可以使用采音板采音,占用PA2~PA6。会多消耗20KB SYS内存
USE_IIS_RECORD    := 0
#############各个算法内存统计###############
#只开识别算法, 只开ASR识别算法HOST END基地址
SDK_ALG_PRO_SRAM_HOST_NOT_ALG_END_ADDR = 0x1FFD3000
#AEC算法算法在BNPU端消耗的内存
AEC_USE_BNPU_SRAM_SIZE	= 30*1024
#自学习算法在BNPU端消耗的内存
CWSL_USE_BNPU_SRAM_SIZE	= 0*1024
#NN降噪算法在NPU端消耗的内存
NN_DENOISE_USE_BNPU_SRAM_SIZE = 47*1024
#TVAD算法在NPU端内存消耗
TVAD_USE_BNPU_SRAM_SIZE = 0*1024
#NN DOA算法在NPU端消耗的内存
NN_DOA_USE_BNPU_SRAM_SIZE = 60*1024
#GCC DOA算法在NPU端消耗的内存
GCC_DOA_USE_BNPU_SRAM_SIZE = 23*1024
#IIS采音在BNPU端内存消耗
IIS_RECORD_USE_BNPU_SRAM_SIZE = 0*1024
