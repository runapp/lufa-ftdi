MCU          = at90usb1287
ARCH         = AVR8
BOARD        = USER
F_CPU        = 8000000
F_USB        = 8000000
OPTIMIZATION = 3
TARGET       = main
LUFA_PATH    = lufa/LUFA
CC_FLAGS     = -DUSE_LUFA_CONFIG_HEADER -Iconfig/ -Wall
#CC_FLAGS     += -Werror
LD_FLAGS     =
SRC          = \
	main.c \
	ftdi.c \
        reset.c \
	$(LUFA_SRC_USB)

# Default target
deftarget: $(TARGET).hex size
.PHONY: deftarget

DEVICE = /dev/serial/by-id/usb-LUFA_FT232_*

# Misc helpers

# Open terminal on USB port
term:
	terminal.py $(DEVICE)
.PHONY: term

# Include LUFA build script makefiles
include $(LUFA_PATH)/Build/lufa_sources.mk
include $(LUFA_PATH)/Build/lufa_build.mk
include $(LUFA_PATH)/Build/lufa_cppcheck.mk
include $(LUFA_PATH)/Build/lufa_avrdude.mk
include $(LUFA_PATH)/Build/lufa_atprogram.mk
include $(LUFA_PATH)/Build/lufa_dfu.mk
include $(LUFA_PATH)/Build/lufa_core.mk
$(LUFA_PATH)/Build/lufa_core.mk:
	@echo 'Did you forget "git submodule init; git submodule update"?'; false

# Remove some stuff from BASE_CC_FLAGS that the LUFA core put in there.
BASE_CC_FLAGS := $(filter-out -fno-inline-small-functions,$(BASE_CC_FLAGS))
