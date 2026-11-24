
#===========================================================
# BNPU 核心算法库联动构建(每个工程独立)
#
#   算法配置文件 : $(PROJECT_ALG_DIR)/user_alg_config.mk
#   配置快照     : $(PROJECT_ALG_DIR)/config.a
#   算法库       : $(PROJECT_ALG_DIR)/ci130x_asr_alg_bnpu.a
#
# 本文件由 projects/<工程>/project_file/makefile 引入, 引入前需已定义:
#   PROJECT_ALG_DIR        本工程 app 目录(相对 project_file, 如 ../app)
#   ALG_PRO_LINK_LDS       本工程 MCU 链接脚本
#   LIB_BNPU_CORE_ALG_PRO  BNPU 算法库文件名
# 并且已经 include 本工程自己的 user_alg_config.mk
#
# 编译本工程时先比对配置文件与快照中各字段的值(不依赖文件时间戳):
#   - 两者一致且算法库已存在: 不重建 BNPU 工程
#   - 不一致, 或快照/算法库缺失: 同步快照后进入 bnpu_core_alg_pro/project_file 重建,
#     算法库输出到本工程 app 目录, 链接与 [1]code.bin 均使用该路径
#===========================================================

BNPU_ALG_PRJ_DIR   := $(SDK_PATH)/components/bnpu_core_alg_pro/project_file
BNPU_ALG_PRJ_NAME  := $(notdir $(patsubst %/,%,$(dir $(BNPU_ALG_PRJ_DIR))))
USER_ALG_CONFIG_MK ?= $(PROJECT_ALG_DIR)/user_alg_config.mk
BNPU_ALG_CONFIG_A  := $(PROJECT_ALG_DIR)/config.a
BNPU_ALG_LIB_FILE  := $(PROJECT_ALG_DIR)/$(LIB_BNPU_CORE_ALG_PRO)
ALG_PRO_LINK_LDS_TEMP ?= ../lds/ci130x_asr_alg_mcu_temp.lds

# BNPU/MCU 两端共用同一套算法内存布局(算法配置决定)
include $(SDK_PATH)/utils/alg_sram_layout.mk

ifneq ($(MAKECMDGOALS), clean)

#-----------------------------------------------------------
# BNPU 算法库: 算法配置变化或算法库缺失时重建
#-----------------------------------------------------------
# 算法库由本工程算法配置决定, 必须先于链接脚本与链接动作生成
$(ALG_PRO_LINK_LDS): $(BNPU_ALG_LIB_FILE)

# 伪目标无产物, 使算法库目标每次编译都进入下面的 recipe 做配置值比对
.PHONY: bnpu-alg-config-check
bnpu-alg-config-check:
	@:

$(BNPU_ALG_LIB_FILE): bnpu-alg-config-check
	@echo 'check alg config: $(notdir $(USER_ALG_CONFIG_MK)) vs config.a'
	@$(LUA) $(SDK_PATH)/utils/parse_user_alg_config_mk.lua check "$(USER_ALG_CONFIG_MK)" "$(BNPU_ALG_CONFIG_A)" ; \
	config_changed=$$? ; \
	if [ $$config_changed -eq 0 ] && [ -f "$(BNPU_ALG_LIB_FILE)" ] ; then \
		echo 'alg config identical to snapshot: bnpu lib is up to date, skip rebuild' ; \
		exit 0 ; \
	fi ; \
	if [ $$config_changed -eq 0 ] ; then \
		echo 'alg config identical to snapshot: bnpu lib missing, rebuild bnpu project' ; \
	else \
		echo 'alg config changed: sync config.a and rebuild bnpu project' ; \
	fi ; \
	$(MAKE) -C "$(BNPU_ALG_PRJ_DIR)" clean PROJECT_NAME=$(BNPU_ALG_PRJ_NAME) || exit 1 ; \
	$(LUA) $(SDK_PATH)/utils/parse_user_alg_config_mk.lua sync "$(USER_ALG_CONFIG_MK)" "$(BNPU_ALG_CONFIG_A)" || exit 1 ; \
	$(MAKE) -C "$(BNPU_ALG_PRJ_DIR)" PROJECT_NAME=$(BNPU_ALG_PRJ_NAME) \
		USER_ALG_CONFIG_MK="$(abspath $(USER_ALG_CONFIG_MK))" \
		BNPU_ALG_OUT_LIB="$(abspath $(BNPU_ALG_LIB_FILE))" || exit 1

#-----------------------------------------------------------
# 链接脚本: 共享 SRAM 分界地址由本工程算法配置决定
#   utils/common.lds 被工程链接脚本 INCLUDE, 必须与当前配置保持一致;
#   两个链接脚本都是每次编译按当前配置重新生成, 内容不变时不覆盖,
#   避免无谓的重新链接
#-----------------------------------------------------------
COMMON_LINK_LDS := $(SDK_PATH)/utils/common.lds

.PHONY: bnpu-alg-lds-force
bnpu-alg-lds-force:
	@:

$(COMMON_LINK_LDS): $(SDK_PATH)/utils/common_temp.lds $(BNPU_ALG_LIB_FILE) bnpu-alg-lds-force
	@sed 's/@@SDK_ALG_PRO_SRAM_HOST_END_ADDR@@/$(SDK_ALG_PRO_SRAM_HOST_END_ADDR)/g' $(SDK_PATH)/utils/common_temp.lds > $@.tmp
	@if cmp -s $@.tmp $@ ; then $(RM) $@.tmp ; else \
		echo 'update $(notdir $@): SDK_ALG_PRO_SRAM_HOST_END_ADDR = $(SDK_ALG_PRO_SRAM_HOST_END_ADDR)' ; \
		mv $@.tmp $@ ; \
	fi

$(ALG_PRO_LINK_LDS): $(ALG_PRO_LINK_LDS_TEMP) $(COMMON_LINK_LDS) bnpu-alg-lds-force
	@sed 's/@@SDK_ALG_PRO_SRAM_HOST_END_ADDR@@/$(SDK_ALG_PRO_SRAM_HOST_END_ADDR)/g' $(ALG_PRO_LINK_LDS_TEMP) > $@.tmp
	@if cmp -s $@.tmp $@ ; then $(RM) $@.tmp ; else \
		echo 'update $(notdir $@): SRAM_END_ADDR = $(SDK_ALG_PRO_SRAM_HOST_END_ADDR)' ; \
		mv $@.tmp $@ ; \
	fi

endif

#-----------------------------------------------------------
# make clean 扩展: clean 本体定义于 utils/common_tail_mcu.mk,
# 这里为其追加前置依赖, 同步删除本工程的算法配置快照与算法库
# (快照缺失视为需重建, 下次编译会重新 sync 并重建 BNPU 工程)
#-----------------------------------------------------------
clean: clean-bnpu-alg-lib

.PHONY: clean-bnpu-alg-lib
clean-bnpu-alg-lib:
	@echo 'Clean bnpu alg config snapshot: $(BNPU_ALG_CONFIG_A)'
	-$(RM) -rf $(BNPU_ALG_CONFIG_A) $(BNPU_ALG_LIB_FILE)
