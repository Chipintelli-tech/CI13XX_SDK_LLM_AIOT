# This file is maked by run generate_makefile.lua
C_FLAGS += -DASR_CODE_VERSION=2
OBJS += build/objs/ci130x_init.o
-include build/objs/ci130x_init.d
build/objs/ci130x_init.o : $(SDK_PATH)/startup/ci130x_init.c
	$(CC_PREFIX)$(CC) $(C_FLAGS) -c -o "$@" "$<"

OBJS += build/objs/ci130x_vtable.o
-include build/objs/ci130x_vtable.d
build/objs/ci130x_vtable.o : $(SDK_PATH)/startup/ci130x_vtable.S
	$(CC_PREFIX)$(AS) $(S_FLAGS) -c -o "$@" "$<"

OBJS += build/objs/ci130x_startup.o
-include build/objs/ci130x_startup.d
build/objs/ci130x_startup.o : $(SDK_PATH)/startup/ci130x_startup.S
	$(CC_PREFIX)$(AS) $(S_FLAGS) -c -o "$@" "$<"

OBJS += build/objs/ci130x_it.o
-include build/objs/ci130x_it.d
build/objs/ci130x_it.o : $(SDK_PATH)/system/ci130x_it.c
	$(CC_PREFIX)$(CC) $(C_FLAGS) -c -o "$@" "$<"

OBJS += build/objs/ci130x_system.o
-include build/objs/ci130x_system.d
build/objs/ci130x_system.o : $(SDK_PATH)/system/ci130x_system.c
	$(CC_PREFIX)$(CC) $(C_FLAGS) -c -o "$@" "$<"

OBJS += build/objs/platform_config_bnpu.o
-include build/objs/platform_config_bnpu.d
build/objs/platform_config_bnpu.o : $(SDK_PATH)/system/platform_config_bnpu.c
	$(CC_PREFIX)$(CC) $(C_FLAGS) -c -o "$@" "$<"

OBJS += build/objs/ci130x_handlers.o
-include build/objs/ci130x_handlers.d
build/objs/ci130x_handlers.o : $(SDK_PATH)/system/ci130x_handlers.c
	$(CC_PREFIX)$(CC) $(C_FLAGS) -c -o "$@" "$<"

OBJS += build/objs/croutine.o
-include build/objs/croutine.d
build/objs/croutine.o : $(SDK_PATH)/components/freertos/croutine.c
	$(CC_PREFIX)$(CC) $(C_FLAGS) -c -o "$@" "$<"

OBJS += build/objs/event_groups.o
-include build/objs/event_groups.d
build/objs/event_groups.o : $(SDK_PATH)/components/freertos/event_groups.c
	$(CC_PREFIX)$(CC) $(C_FLAGS) -c -o "$@" "$<"

OBJS += build/objs/list.o
-include build/objs/list.d
build/objs/list.o : $(SDK_PATH)/components/freertos/list.c
	$(CC_PREFIX)$(CC) $(C_FLAGS) -c -o "$@" "$<"

OBJS += build/objs/queue.o
-include build/objs/queue.d
build/objs/queue.o : $(SDK_PATH)/components/freertos/queue.c
	$(CC_PREFIX)$(CC) $(C_FLAGS) -c -o "$@" "$<"

OBJS += build/objs/stream_buffer.o
-include build/objs/stream_buffer.d
build/objs/stream_buffer.o : $(SDK_PATH)/components/freertos/stream_buffer.c
	$(CC_PREFIX)$(CC) $(C_FLAGS) -c -o "$@" "$<"

OBJS += build/objs/tasks.o
-include build/objs/tasks.d
build/objs/tasks.o : $(SDK_PATH)/components/freertos/tasks.c
	$(CC_PREFIX)$(CC) $(C_FLAGS) -c -o "$@" "$<"

OBJS += build/objs/timers.o
-include build/objs/timers.d
build/objs/timers.o : $(SDK_PATH)/components/freertos/timers.c
	$(CC_PREFIX)$(CC) $(C_FLAGS) -c -o "$@" "$<"

OBJS += build/objs/heap_4.o
-include build/objs/heap_4.d
build/objs/heap_4.o : $(SDK_PATH)/components/freertos/portable/MemMang/heap_4.c
	$(CC_PREFIX)$(CC) $(C_FLAGS) -c -o "$@" "$<"

OBJS += build/objs/heap_644.o
-include build/objs/heap_644.d
build/objs/heap_644.o : $(SDK_PATH)/components/freertos/portable/MemMang/heap_644.c
	$(CC_PREFIX)$(CC) $(C_FLAGS) -c -o "$@" "$<"

OBJS += build/objs/ci_log.o
-include build/objs/ci_log.d
build/objs/ci_log.o : $(SDK_PATH)/components/log/ci_log.c
	$(CC_PREFIX)$(CC) $(C_FLAGS) -c -o "$@" "$<"

OBJS += build/objs/ci_basic_alg.o
-include build/objs/ci_basic_alg.d
build/objs/ci_basic_alg.o : $(SDK_PATH)/components/alg/basic_alg/ci_basic_alg.c
	$(CC_PREFIX)$(CC) $(C_FLAGS) -c -o "$@" "$<"

OBJS += build/objs/alg_preprocess.o
-include build/objs/alg_preprocess.d
build/objs/alg_preprocess.o : $(SDK_PATH)/components/audio_in_manage/alg_preprocess.c
	$(CC_PREFIX)$(CC) $(C_FLAGS) -c -o "$@" "$<"

OBJS += build/objs/flash_manage.o
-include build/objs/flash_manage.d
build/objs/flash_manage.o : $(SDK_PATH)/components/asr/nn_and_flash/nn_and_flash_src/flash_manage.c
	$(CC_PREFIX)$(CC) $(C_FLAGS) -c -o "$@" "$<"

OBJS += build/objs/flash_manage_inner_port.o
-include build/objs/flash_manage_inner_port.d
build/objs/flash_manage_inner_port.o : $(SDK_PATH)/components/asr/nn_and_flash/nn_and_flash_src/flash_manage_inner_port.c
	$(CC_PREFIX)$(CC) $(C_FLAGS) -c -o "$@" "$<"

OBJS += build/objs/nn_and_flash_manage.o
-include build/objs/nn_and_flash_manage.d
build/objs/nn_and_flash_manage.o : $(SDK_PATH)/components/asr/nn_and_flash/nn_and_flash_src/nn_and_flash_manage.c
	$(CC_PREFIX)$(CC) $(C_FLAGS) -c -o "$@" "$<"

OBJS += build/objs/debug_time_consuming.o
-include build/objs/debug_time_consuming.d
build/objs/debug_time_consuming.o : $(SDK_PATH)/components/assist/debug_time_consuming.c
	$(CC_PREFIX)$(CC) $(C_FLAGS) -c -o "$@" "$<"

OBJS += build/objs/led_light_control.o
-include build/objs/led_light_control.d
build/objs/led_light_control.o : $(SDK_PATH)/components/led/led_light_control.c
	$(CC_PREFIX)$(CC) $(C_FLAGS) -c -o "$@" "$<"

OBJS += build/objs/codec_manage_outside_port.o
-include build/objs/codec_manage_outside_port.d
build/objs/codec_manage_outside_port.o : $(SDK_PATH)/components/codec_manager/codec_manage_outside_port.c
	$(CC_PREFIX)$(CC) $(C_FLAGS) -c -o "$@" "$<"

OBJS += build/objs/status_share.o
-include build/objs/status_share.d
build/objs/status_share.o : $(SDK_PATH)/components/status_share/status_share.c
	$(CC_PREFIX)$(CC) $(C_FLAGS) -c -o "$@" "$<"

OBJS += build/objs/ci130x_core_eclic.o
-include build/objs/ci130x_core_eclic.d
build/objs/ci130x_core_eclic.o : $(SDK_PATH)/driver/ci130x_chip_driver/src/ci130x_core_eclic.c
	$(CC_PREFIX)$(CC) $(C_FLAGS) -c -o "$@" "$<"

OBJS += build/objs/ci130x_core_timer.o
-include build/objs/ci130x_core_timer.d
build/objs/ci130x_core_timer.o : $(SDK_PATH)/driver/ci130x_chip_driver/src/ci130x_core_timer.c
	$(CC_PREFIX)$(CC) $(C_FLAGS) -c -o "$@" "$<"

OBJS += build/objs/ci130x_dma.o
-include build/objs/ci130x_dma.d
build/objs/ci130x_dma.o : $(SDK_PATH)/driver/ci130x_chip_driver/src/ci130x_dma.c
	$(CC_PREFIX)$(CC) $(C_FLAGS) -c -o "$@" "$<"

OBJS += build/objs/ci130x_spiflash.o
-include build/objs/ci130x_spiflash.d
build/objs/ci130x_spiflash.o : $(SDK_PATH)/driver/ci130x_chip_driver/src/ci130x_spiflash.c
	$(CC_PREFIX)$(CC) $(C_FLAGS) -c -o "$@" "$<"

OBJS += build/objs/ci130x_dtrflash.o
-include build/objs/ci130x_dtrflash.d
build/objs/ci130x_dtrflash.o : $(SDK_PATH)/driver/ci130x_chip_driver/src/ci130x_dtrflash.c
	$(CC_PREFIX)$(CC) $(C_FLAGS) -c -o "$@" "$<"

OBJS += build/objs/ci130x_core_misc.o
-include build/objs/ci130x_core_misc.d
build/objs/ci130x_core_misc.o : $(SDK_PATH)/driver/ci130x_chip_driver/src/ci130x_core_misc.c
	$(CC_PREFIX)$(CC) $(C_FLAGS) -c -o "$@" "$<"

OBJS += build/objs/ci130x_timer.o
-include build/objs/ci130x_timer.d
build/objs/ci130x_timer.o : $(SDK_PATH)/driver/ci130x_chip_driver/src/ci130x_timer.c
	$(CC_PREFIX)$(CC) $(C_FLAGS) -c -o "$@" "$<"

OBJS += build/objs/ci130x_uart.o
-include build/objs/ci130x_uart.d
build/objs/ci130x_uart.o : $(SDK_PATH)/driver/ci130x_chip_driver/src/ci130x_uart.c
	$(CC_PREFIX)$(CC) $(C_FLAGS) -c -o "$@" "$<"

OBJS += build/objs/board.o
-include build/objs/board.d
build/objs/board.o : $(SDK_PATH)/driver/boards/board.c
	$(CC_PREFIX)$(CC) $(C_FLAGS) -c -o "$@" "$<"

OBJS += build/objs/ci_doa_apply.o
-include build/objs/ci_doa_apply.d
build/objs/ci_doa_apply.o : $(SDK_PATH)/components/alg/doa/ci_doa_apply.c
	$(CC_PREFIX)$(CC) $(C_FLAGS) -c -o "$@" "$<"

OBJS += build/objs/main.o
-include build/objs/main.d
build/objs/main.o : $(SDK_PATH)/components/bnpu_core_alg_pro/src/main.c
	$(CC_PREFIX)$(CC) $(C_FLAGS) -c -o "$@" "$<"

LIB_FILES += $(SDK_PATH)/$(LIBS_PATH)/libnewlib_port_bnpu.a
LD_FLAGS += -L$(SDK_PATH)/$(LIBS_PATH)
LIBS += -lnewlib_port_bnpu
LIB_FILES += $(SDK_PATH)/$(LIBS_PATH)/libflash_encrypt.a
LD_FLAGS += -L$(SDK_PATH)/$(LIBS_PATH)
LIBS += -lflash_encrypt
LIB_FILES += $(SDK_PATH)/$(LIBS_PATH)/libbnpu_math.a
LD_FLAGS += -L$(SDK_PATH)/$(LIBS_PATH)
LIBS += -lbnpu_math
LIB_FILES += $(SDK_PATH)/$(LIBS_PATH)/libbnpu_mem_manage.a
LD_FLAGS += -L$(SDK_PATH)/$(LIBS_PATH)
LIBS += -lbnpu_mem_manage
LIB_FILES += $(SDK_PATH)/$(LIBS_PATH)/libdsu.a
LD_FLAGS += -L$(SDK_PATH)/$(LIBS_PATH)
LIBS += -ldsu
LIB_FILES += $(SDK_PATH)/$(LIBS_PATH)/libfreertos_port.a
LD_FLAGS += -L$(SDK_PATH)/$(LIBS_PATH)
LIBS += -lfreertos_port
LIB_FILES += $(SDK_PATH)/$(LIBS_PATH)/libalc_auto_switch.a
LD_FLAGS += -L$(SDK_PATH)/$(LIBS_PATH)
LIBS += -lalc_auto_switch
LIB_FILES += $(SDK_PATH)/$(LIBS_PATH)/libtra_denoise.a
LD_FLAGS += -L$(SDK_PATH)/$(LIBS_PATH)
LIBS += -ltra_denoise
LIB_FILES += $(SDK_PATH)/$(LIBS_PATH)/libnn_denoise_v3.a
LD_FLAGS += -L$(SDK_PATH)/$(LIBS_PATH)
LIBS += -lnn_denoise_v3
LIB_FILES += $(SDK_PATH)/$(LIBS_PATH)/libaec.a
LD_FLAGS += -L$(SDK_PATH)/$(LIBS_PATH)
LIBS += -laec
LIB_FILES += $(SDK_PATH)/$(LIBS_PATH)/libtvad.a
LD_FLAGS += -L$(SDK_PATH)/$(LIBS_PATH)
LIBS += -ltvad
LIB_FILES += $(SDK_PATH)/$(LIBS_PATH)/libbf.a
LD_FLAGS += -L$(SDK_PATH)/$(LIBS_PATH)
LIBS += -lbf
LIB_FILES += $(SDK_PATH)/$(LIBS_PATH)/libderverb.a
LD_FLAGS += -L$(SDK_PATH)/$(LIBS_PATH)
LIBS += -lderverb
LIB_FILES += $(SDK_PATH)/$(LIBS_PATH)/libnn_sed.a
LD_FLAGS += -L$(SDK_PATH)/$(LIBS_PATH)
LIBS += -lnn_sed
LIB_FILES += $(SDK_PATH)/$(LIBS_PATH)/libfft.a
LD_FLAGS += -L$(SDK_PATH)/$(LIBS_PATH)
LIBS += -lfft
LIB_FILES += $(SDK_PATH)/$(LIBS_PATH)/libpwk.a
LD_FLAGS += -L$(SDK_PATH)/$(LIBS_PATH)
LIBS += -lpwk
LIB_FILES += $(SDK_PATH)/$(LIBS_PATH)/libaes.a
LD_FLAGS += -L$(SDK_PATH)/$(LIBS_PATH)
LIBS += -laes
LIB_FILES += $(SDK_PATH)/$(LIBS_PATH)/libnn_vpr.a
LD_FLAGS += -L$(SDK_PATH)/$(LIBS_PATH)
LIBS += -lnn_vpr
LIB_FILES += $(SDK_PATH)/$(LIBS_PATH)/libnn_wman_vpr.a
LD_FLAGS += -L$(SDK_PATH)/$(LIBS_PATH)
LIBS += -lnn_wman_vpr
LIB_FILES += $(SDK_PATH)/$(LIBS_PATH)/libasr_bnpu_dis_deepse_v2.a
LD_FLAGS += -L$(SDK_PATH)/$(LIBS_PATH)
LIBS += -lasr_bnpu_dis_deepse_v2
LIB_FILES += $(SDK_PATH)/$(LIBS_PATH)/libasr_bnpu_use_deepse_v2.a
LD_FLAGS += -L$(SDK_PATH)/$(LIBS_PATH)
LIBS += -lasr_bnpu_use_deepse_v2
LIB_FILES += $(SDK_PATH)/$(LIBS_PATH)/libgcc_doa.a
LD_FLAGS += -L$(SDK_PATH)/$(LIBS_PATH)
LIBS += -lgcc_doa
LIB_FILES += $(SDK_PATH)/$(LIBS_PATH)/libnn_doa.a
LD_FLAGS += -L$(SDK_PATH)/$(LIBS_PATH)
LIBS += -lnn_doa
C_FLAGS += -I$(SDK_PATH)/driver/ci130x_chip_driver/inc
C_FLAGS += -I$(SDK_PATH)/driver/boards
C_FLAGS += -I$(SDK_PATH)/driver/third_device_driver/outside_codec
C_FLAGS += -I$(SDK_PATH)/system
C_FLAGS += -I$(SDK_PATH)/components/log
C_FLAGS += -I$(SDK_PATH)/components/tvad
C_FLAGS += -I$(SDK_PATH)/components/assist
C_FLAGS += -I$(SDK_PATH)/components/freertos/include
C_FLAGS += -I$(SDK_PATH)/components/freertos/portable/GCC/N307
C_FLAGS += -I$(SDK_PATH)/components
C_FLAGS += -I$(SDK_PATH)/components/asr
C_FLAGS += -I$(SDK_PATH)/components/asr/decoder_v2
C_FLAGS += -I$(SDK_PATH)/components/asr/asr_top
C_FLAGS += -I$(SDK_PATH)/components/asr/asr_top/asr_top_inc
C_FLAGS += -I$(SDK_PATH)/components/asr/asr_top/asr_top_src
C_FLAGS += -I$(SDK_PATH)/components/asr/decoder_v2/decoder_inc
C_FLAGS += -I$(SDK_PATH)/components/npu/npu_inc
C_FLAGS += -I$(SDK_PATH)/components/asr/vad_fe/vad_fe_inc
C_FLAGS += -I$(SDK_PATH)/components/asr/nn_and_flash
C_FLAGS += -I$(SDK_PATH)/components/asr/nn_and_flash/nn_and_flash_inc
C_FLAGS += -I$(SDK_PATH)/components/alg
C_FLAGS += -I$(SDK_PATH)/components/alg/tra_denoise
C_FLAGS += -I$(SDK_PATH)/components/alg/nn_denoise
C_FLAGS += -I$(SDK_PATH)/components/alg/alc_auto_switch
C_FLAGS += -I$(SDK_PATH)/components/alg/basic_alg
C_FLAGS += -I$(SDK_PATH)/components/alg/aec
C_FLAGS += -I$(SDK_PATH)/components/alg/beamforming
C_FLAGS += -I$(SDK_PATH)/components/alg/dereverb
C_FLAGS += -I$(SDK_PATH)/components/alg/doa
C_FLAGS += -I$(SDK_PATH)/components/nuclear_com/
C_FLAGS += -I$(SDK_PATH)/components/Newlib_Port
C_FLAGS += -I$(SDK_PATH)/components/fft
C_FLAGS += -I$(SDK_PATH)/components/flash_control/flash_control_inc
C_FLAGS += -I$(SDK_PATH)/components/flash_control/flash_control_outside
C_FLAGS += -I$(SDK_PATH)/components/codec_manager
C_FLAGS += -I$(SDK_PATH)/components/ci_nvdm
C_FLAGS += -I$(SDK_PATH)/components/cmd_info
C_FLAGS += -I$(SDK_PATH)/components/sys_monitor
C_FLAGS += -I$(SDK_PATH)/components/ota
C_FLAGS += -I$(SDK_PATH)/components/audio_pre_rslt_iis_out
C_FLAGS += -I$(SDK_PATH)/components/audio_in_manage
C_FLAGS += -I$(SDK_PATH)/components/assist/SEGGER
C_FLAGS += -I$(SDK_PATH)/components/assist/SEGGER/config
C_FLAGS += -I$(SDK_PATH)/components/nuclear_com
C_FLAGS += -I$(SDK_PATH)/components/status_share
C_FLAGS += -I$(SDK_PATH)/components/player/audio_play
C_FLAGS += -I$(SDK_PATH)/components/simple_audio_player
C_FLAGS += -I$(SDK_PATH)/components/bnpu_core_alg_pro/src
C_FLAGS += -I$(SDK_PATH)/components/alg/sound_event_detection
C_FLAGS += -I$(SDK_PATH)/components/aes/
C_FLAGS += -I$(SDK_PATH)/components/alg/vp/
C_FLAGS += -I$(SDK_PATH)/components/alg/nn_denoise
C_FLAGS += -I$(SDK_PATH)/components/alg/pwk/
C_FLAGS += -I$(SDK_PATH)/components/nmsis/include
C_FLAGS += -I$(SDK_PATH)/components/ci_common/
C_FLAGS += -I$(SDK_PATH)/components/alg/doa/gcc_doa/
C_FLAGS += -I$(SDK_PATH)/components/alg/doa/nn_doa/
C_FLAGS += -I$(SDK_PATH)/components/dnn/dnn_cbb
C_FLAGS += -I$(SDK_PATH)/components/dnn/asr_dnn
C_FLAGS += -I$(SDK_PATH)/components/dnn/tdnn
C_FLAGS += -I$(SDK_PATH)/components/dnn/cnn
C_FLAGS += -I$(SDK_PATH)/utils
