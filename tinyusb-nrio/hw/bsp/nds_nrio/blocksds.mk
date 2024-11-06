include $(TINYUSB_PATH)/src/tinyusb.mk

TINYUSB_DEFINES := -DCFG_TUSB_MCU=OPT_MCU_NRIO
TINYUSB_INCLUDEDIRS := $(TINYUSB_PATH)/hw $(TINYUSB_PATH)/src
TINYUSB_SOURCES := $(addprefix $(TINYUSB_PATH)/,$(TINYUSB_SRC_C))
TINYUSB_SOURCES += $(TINYUSB_PATH)/src/portable/nrio/dcd_nrio.c
TINYUSB_SOURCES_ASM := $(TINYUSB_PATH)/src/portable/nrio/dcd_nrio_asm.s

ifeq ($(TINYUSB_DEBUG),$(filter $(TINYUSB_DEBUG),0 1 2 3))
TINYUSB_DEFINES	+= -DCFG_TUSB_DEBUG=$(TINYUSB_DEBUG)
endif

DEFINES += $(TINYUSB_DEFINES)
INCLUDEDIRS += $(TINYUSB_INCLUDEDIRS)
SOURCES_S += $(TINYUSB_SOURCES_ASM)
SOURCES_C += $(TINYUSB_SOURCES)