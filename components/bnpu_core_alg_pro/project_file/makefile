

SDK_PATH ?= ../../..
BUILD_DIR := build

# 算法功能配置文件: 上层工程编译时传入(即该工程 app 目录下的 user_alg_config.mk),
# 单独编译本工程(不指定该变量)时回退到 SDK 公共配置
#USER_ALG_CONFIG_MK ?= $(SDK_PATH)/system/user_alg_config.mk
include $(USER_ALG_CONFIG_MK)
# BNPU 端算法内存占用统计(与 MCU 工程共用同一套内存布局)
include $(SDK_PATH)/utils/alg_sram_layout.mk
include $(SDK_PATH)/utils/common_head.mk


ifeq ($(USE_ASR), 1)
	C_FLAGS += -DCINN_MAX_CHA_NUM=1 -DNO_ASR_FLOW=0
else
	C_FLAGS += -DNO_ASR_FLOW=1
endif
ifeq ($(USE_ANY_MIC), 1)
	C_FLAGS += -DUSE_DUAL_MIC_ANY=1 -DHOST_CODEC_CHA_NUM=2 -DOFFLINE_DUAL_MIC_ALG_SUPPORT=1
endif
ifeq ($(USE_AEC), 1)
	C_FLAGS += -DUSE_AEC_MODULE=1 -DCINN_MAX_CHA_NUM=1   
endif
ifeq ($(USE_CWSL), 1)
	C_FLAGS += -DUSE_CWSL=1   -DASR_FE_REDUCE_MEM=1 -DCINN_MAX_CHA_NUM=1 
endif
ifeq ($(USE_NN_DOA), 1)
	C_FLAGS += -DUSE_NN_DOA=1 -DASR_FE_REDUCE_MEM=1 -DOFFLINE_DUAL_MIC_ALG_SUPPORT=1 -DHOST_CODEC_CHA_NUM=2  -DCINN_MAX_CHA_NUM=1 
endif
ifeq ($(USE_GCC_DOA), 1)
	C_FLAGS += -DUSE_GCC_DOA=1   -DASR_FE_REDUCE_MEM=1 -DOFFLINE_DUAL_MIC_ALG_SUPPORT=1 -DHOST_CODEC_CHA_NUM=2 -DCINN_MAX_CHA_NUM=1 
endif
ifeq ($(USE_DEREVERB), 1)
	C_FLAGS += -DOFFLINE_DUAL_MIC_ALG_SUPPORT=1 -DUSE_DEREVERB_MODULE=1 -DASR_FE_REDUCE_MEM=1 -DHOST_CODEC_CHA_NUM=2 -DCINN_MAX_CHA_NUM=1 
endif

ifeq ($(USE_NN_DENOISE), 1)
	C_FLAGS += -DUSE_NN_DENOISE=1   -DCINN_MAX_CHA_NUM=1 
endif

ifeq ($(USE_NN_VAD), 1)
	C_FLAGS += -DUSE_NN_VAD=1   
endif


ifeq ($(USE_TRA_DENOISE), 1)
	C_FLAGS += -DUSE_TRA_DENOISE=1   -DCINN_MAX_CHA_NUM=1 
endif

ifeq ($(USE_VPR), 1)  
	C_FLAGS += -DUSE_VPR=1 -DCINN_MAX_CHA_NUM=1 -DASR_FE_REDUCE_MEM=0
endif
ifeq ($(USE_WMAN_VPR), 1)  
	C_FLAGS += -DUSE_WMAN_VPR=1 -DCINN_MAX_CHA_NUM=1 -DASR_FE_REDUCE_MEM=0
endif
ifeq ($(USE_BF), 1) 	
	C_FLAGS += -DDEEP_SEPARATE_ENABLE=1 -DCINN_MAX_CHA_NUM=2  -DASR_FE_REDUCE_MEM=1 -DUSE_BEAMFORMING_MODULE=1 -DOFFLINE_DUAL_MIC_ALG_SUPPORT=1 -DHOST_CODEC_CHA_NUM=2
endif
ifeq ($(USE_SED_CRY), 1) 
	C_FLAGS += -DUSE_SED=1 -DNO_ASR_FLOW=1 -DCINN_MAX_CHA_NUM=1 -DASR_FE_REDUCE_MEM=0
endif
ifeq ($(USE_SED_SNORE), 1) 
	C_FLAGS += -DUSE_SED=1 -DNO_ASR_FLOW=1 -DCINN_MAX_CHA_NUM=1 -DASR_FE_REDUCE_MEM=0	
endif
ifeq ($(USE_IIS_RECORD), 1)
C_FLAGS += -DUSE_IIS1_OUT_PRE_RSLT_AUDIO=1
endif

ifeq ($(USE_BF), 1)
	LIBS := -lasr_bnpu_use_deepse_v2
else
	LIBS := -lasr_bnpu_dis_deepse_v2
endif


COMMON_LINK_LDS := ../../../utils/common.lds
ALG_PRO_LINK_LDS := ../lds/ci130x_asr_alg_bnpu.lds
# 算法库输出路径: 上层工程编译时传入该工程 app 目录下的路径, 使每个工程独立保存
# 自己的 ci130x_asr_alg_bnpu.a; 单独编译本工程时输出到 SDK 公共库目录
BNPU_ALG_OUT_LIB ?= $(SDK_PATH)/$(LIBS_PATH)/ci130x_asr_alg_bnpu.a
SECONDARY_FLASH := $(BNPU_ALG_OUT_LIB)
SECONDARY_LIST := $(BUILD_DIR)/$(PROJECT_NAME).lst

LIBS += -lm

LTO_OPTION := -flto
O_OPTION += -Os

include $(SDK_PATH)/utils/common_flags.mk

C_FLAGS += $(LTO_OPTION) $(O_OPTION) -DCI_CONFIG_FILE=\"user_config.h\"  -DCORE_ID=1
S_FLAGS += $(LTO_OPTION) $(O_OPTION)
S_FLAGS += -I$(SDK_PATH)/components -I$(SDK_PATH)/driver/ci130x_chip_driver/inc


LD_FLAGS := -march=rv32imafc -mabi=ilp32f -mcmodel=medlow -msmall-data-limit=8 -msave-restore -mfdiv $(O_OPTION) -fmessage-length=0	$(LTO_OPTION)
LD_FLAGS += -fsigned-char -ffunction-sections -fdata-sections -fno-common -fno-delete-null-pointer-checks -fno-unroll-loops
LD_FLAGS += -g -T "$(ALG_PRO_LINK_LDS)"  -nostartfiles -Xlinker --gc-sections -Wl,-Map,"$(BUILD_DIR)/$(PROJECT_NAME).map" --specs=nano.specs

LD_FLAGS += -Wl,--wrap,memcmp
LD_FLAGS += -Wl,--wrap,memcpy
LD_FLAGS += -Wl,--wrap,memmove
LD_FLAGS += -Wl,--wrap,memset
LD_FLAGS += -Wl,--wrap,strcat
LD_FLAGS += -Wl,--wrap,strcmp
LD_FLAGS += -Wl,--wrap,strcpy
LD_FLAGS += -Wl,--wrap,strlen
LD_FLAGS += -Wl,--wrap,strncat
LD_FLAGS += -Wl,--wrap,strncmp
LD_FLAGS += -Wl,--wrap,strncpy
LD_FLAGS += -Wl,--wrap,strstr
LD_FLAGS += -Wl,--wrap,stpcpy
LD_FLAGS += -Wl,--wrap,stpncpy
LD_FLAGS += -Wl,--wrap,strnlen
LD_FLAGS += -Wl,--wrap,strtoul
LD_FLAGS += -Wl,--wrap,atol
LD_FLAGS += -Wl,--wrap,qsort
# LD_FLAGS += -Wl,--wrap,malloc
# LD_FLAGS += -Wl,--wrap,free
LD_FLAGS += -Wl,--wrap,vsnprintf
LD_FLAGS += -Wl,--wrap,sprintf
LD_FLAGS += -Wl,--wrap,sscanf
LD_FLAGS += -Wl,--wrap,abs
LD_FLAGS += -Wl,--wrap,isnanf
LD_FLAGS += -Wl,--wrap,isnan
LD_FLAGS += -Wl,--wrap,isinff
LD_FLAGS += -Wl,--wrap,isinf
LD_FLAGS += -Wl,--wrap,cosf
LD_FLAGS += -Wl,--wrap,sinf
LD_FLAGS += -Wl,--wrap,tanf
LD_FLAGS += -Wl,--wrap,expf
LD_FLAGS += -Wl,--wrap,sqrtf
LD_FLAGS += -Wl,--wrap,fabsf
LD_FLAGS += -Wl,--wrap,logf
LD_FLAGS += -Wl,--wrap,log10f
LD_FLAGS += -Wl,--wrap,atoi

LD_FLAGS += -L$(SDK_PATH)/$(LIBS_PATH) -L$(SDK_PATH)/components/freertos/portable/GCC -L$(SDK_PATH)/utils

all: secondary-outputs 

ifneq ($(MAKECMDGOALS), clean)
-include $(BUILD_DIR)/source_file.mk
endif
$(ALG_PRO_LINK_LDS):../lds/ci130x_asr_alg_bnpu_temp.lds
	@echo 'mk ci130x_asr_alg_bnpu.lds'
	@echo "ALG_BPU_SRAM_SIZE_TOTAL_CNT = $(ALG_BPU_SRAM_SIZE_TOTAL_CNT)"
	@echo "AEC_NN_DOA_MAX_MEM_SIZE = $(AEC_NN_DOA_MAX_MEM_SIZE)"
	sed 's/@@SDK_ALG_PRO_SRAM_HOST_END_ADDR@@/$(SDK_ALG_PRO_SRAM_HOST_END_ADDR)/g' $< >$@
$(COMMON_LINK_LDS):../../../utils/common_temp.lds
	@echo 'mk common.lds'
	sed 's/@@SDK_ALG_PRO_SRAM_HOST_END_ADDR@@/$(SDK_ALG_PRO_SRAM_HOST_END_ADDR)/g' $< >$@



$(BUILD_DIR)/$(PROJECT_NAME).elf: $(OBJS) $(USER_OBJS) build/source_file.mk $(LIB_FILES) $(ALG_PRO_LINK_LDS) makefile
	@echo 'Building target: $@'
	@echo 'Invoking: GNU RISC-V Cross C++ Linker'
	$(CC_PREFIX)$(LD) $(LD_FLAGS) -o "$(BUILD_DIR)/$(PROJECT_NAME).elf" $(OBJS) $(USER_OBJS) $(LIBS) 
	@echo 'Finished building target: $@'
	@echo ' '

$(SECONDARY_FLASH): $(BUILD_DIR)/$(PROJECT_NAME).elf
	@echo 'Invoking: GNU RISC-V Cross Create Flash Image'
	@mkdir -p $(dir $(SECONDARY_FLASH)) ../firmware/user_code
	$(CC_PREFIX)$(OC) -O binary "$(BUILD_DIR)/$(PROJECT_NAME).elf" "$(SECONDARY_FLASH)"
	@echo 'Finished building: $@'
	@echo ' '

$(BUILD_DIR)/$(PROJECT_NAME).lst: $(BUILD_DIR)/$(PROJECT_NAME).elf
	@echo 'Invoking: GNU RISC-V Cross Create Listing'
	$(CC_PREFIX)$(OD) --demangle --disassemble --reloc --wide "$(BUILD_DIR)/$(PROJECT_NAME).elf" > "$(BUILD_DIR)/$(PROJECT_NAME).lst"
	$(CC_PREFIX)$(SIZE) --format=berkeley "$(BUILD_DIR)/$(PROJECT_NAME).elf"
	@echo 'Finished building: $@'
	@echo ' '


secondary-outputs: $(ALG_PRO_LINK_LDS) $(COMMON_LINK_LDS) $(SECONDARY_FLASH) $(SECONDARY_LIST) 

include $(SDK_PATH)/utils/common_tail_bnpu.mk

