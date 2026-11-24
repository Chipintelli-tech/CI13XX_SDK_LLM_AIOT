
#===========================================================
# BNPU 算法内存占用统计与 HOST 端 SRAM 结束地址计算
#
# user_alg_config.mk 决定启用哪些算法, 这里把各算法在 BNPU 端占用的内存累加为
# ALG_BPU_SRAM_SIZE_TOTAL_CNT, 再用 SDK_ALG_PRO_SRAM_HOST_NOT_ALG_END_ADDR 减去
# 该值, 得到 HOST 端可用 SRAM 的结束地址 SDK_ALG_PRO_SRAM_HOST_END_ADDR。
#
# 注意: BNPU 工程(bnpu_core_alg_pro)与 MCU 工程(projects/xxx)都必须 include
#       本文件, 且必须在 include 各自的 user_alg_config.mk 之后,
#       保证两个核心使用同一套内存布局。
#===========================================================
ALG_BPU_SRAM_SIZE_TOTAL_CNT := 0

ifeq ($(USE_AEC), 1)
ALG_BPU_SRAM_SIZE_TOTAL_CNT := 0x$(shell let add=$(ALG_BPU_SRAM_SIZE_TOTAL_CNT)+$(AEC_USE_BNPU_SRAM_SIZE); printf '%x' $$add )
endif
ifeq ($(USE_CWSL), 1)
ALG_BPU_SRAM_SIZE_TOTAL_CNT := 0x$(shell let add=$(ALG_BPU_SRAM_SIZE_TOTAL_CNT)+$(CWSL_USE_BNPU_SRAM_SIZE); printf '%x' $$add )
endif
ifeq ($(USE_NN_DOA), 1)
ALG_BPU_SRAM_SIZE_TOTAL_CNT := 0x$(shell let add=$(ALG_BPU_SRAM_SIZE_TOTAL_CNT)+$(NN_DOA_USE_BNPU_SRAM_SIZE); printf '%x' $$add )
endif
ifeq ($(USE_GCC_DOA), 1)
ALG_BPU_SRAM_SIZE_TOTAL_CNT := 0x$(shell let add=$(ALG_BPU_SRAM_SIZE_TOTAL_CNT)+$(GCC_DOA_USE_BNPU_SRAM_SIZE); printf '%x' $$add )
endif
ifeq ($(USE_DEREVERB), 1)
ALG_BPU_SRAM_SIZE_TOTAL_CNT := 0x$(shell let add=$(ALG_BPU_SRAM_SIZE_TOTAL_CNT)+$(DEREVERB_USE_BNPU_SRAM_SIZE); printf '%x' $$add )
endif
ifeq ($(USE_NN_DENOISE), 1)
ALG_BPU_SRAM_SIZE_TOTAL_CNT := 0x$(shell let add=$(ALG_BPU_SRAM_SIZE_TOTAL_CNT)+$(NN_DENOISE_USE_BNPU_SRAM_SIZE); printf '%x' $$add )
endif
ifeq ($(USE_NN_VAD), 1)
ALG_BPU_SRAM_SIZE_TOTAL_CNT := 0x$(shell let add=$(ALG_BPU_SRAM_SIZE_TOTAL_CNT)+$(TVAD_USE_BNPU_SRAM_SIZE); printf '%x' $$add )
endif
ifeq ($(USE_TRA_DENOISE), 1)
ALG_BPU_SRAM_SIZE_TOTAL_CNT := 0x$(shell let add=$(ALG_BPU_SRAM_SIZE_TOTAL_CNT)+$(TRA_DENOISE_USE_BNPU_SRAM_SIZE); printf '%x' $$add )
endif
ifeq ($(USE_VPR), 1)
ALG_BPU_SRAM_SIZE_TOTAL_CNT := 0x$(shell let add=$(ALG_BPU_SRAM_SIZE_TOTAL_CNT)+$(NN_VPR_USE_BNPU_SRAM_SIZE); printf '%x' $$add )
endif
ifeq ($(USE_WMAN_VPR), 1)
ALG_BPU_SRAM_SIZE_TOTAL_CNT := 0x$(shell let add=$(ALG_BPU_SRAM_SIZE_TOTAL_CNT)+$(NN_VPR_USE_BNPU_SRAM_SIZE); printf '%x' $$add )
endif
ifeq ($(USE_BF), 1)
ALG_BPU_SRAM_SIZE_TOTAL_CNT := 0x$(shell let add=$(ALG_BPU_SRAM_SIZE_TOTAL_CNT)+$(BF_USE_BNPU_SRAM_SIZE); printf '%x' $$add )
endif
ifeq ($(USE_SED_CRY), 1)
ALG_BPU_SRAM_SIZE_TOTAL_CNT := 0x$(shell let add=$(ALG_BPU_SRAM_SIZE_TOTAL_CNT)+$(NN_SED_USE_BNPU_SRAM_SIZE); printf '%x' $$add )
endif
ifeq ($(USE_SED_SNORE), 1)
ALG_BPU_SRAM_SIZE_TOTAL_CNT := 0x$(shell let add=$(ALG_BPU_SRAM_SIZE_TOTAL_CNT)+$(NN_SED_USE_BNPU_SRAM_SIZE); printf '%x' $$add )
endif
ifeq ($(USE_IIS_RECORD), 1)
ALG_BPU_SRAM_SIZE_TOTAL_CNT := 0x$(shell let add=$(ALG_BPU_SRAM_SIZE_TOTAL_CNT)+$(IIS_RECORD_USE_BNPU_SRAM_SIZE); printf '%x' $$add )
endif

SDK_ALG_PRO_SRAM_HOST_END_ADDR := 0x$(shell let sub=$(SDK_ALG_PRO_SRAM_HOST_NOT_ALG_END_ADDR)-$(ALG_BPU_SRAM_SIZE_TOTAL_CNT); printf '%x' $$sub )
