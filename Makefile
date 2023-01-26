#*******************************************************************************
#   Ledger App
#   (c) 2017 Ledger
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#*******************************************************************************

ifeq ($(BOLOS_SDK),)
$(error Environment variable BOLOS_SDK is not set)
endif
include $(BOLOS_SDK)/Makefile.defines

APP_LOAD_PARAMS  = --curve ed25519
ifeq ($(TARGET_NAME), TARGET_NANOX)
    APP_LOAD_PARAMS += --appFlags 0x200  # APPLICATION_FLAG_BOLOS_SETTINGS
else
    APP_LOAD_PARAMS += --appFlags 0x000
endif
APP_LOAD_PARAMS += $(COMMON_LOAD_PARAMS)

# Pending review parameters
APP_LOAD_PARAMS += --tlvraw 9F:01
DEFINES += HAVE_PENDING_REVIEW_SCREEN

##################
# Define Version #
##################

APPVERSION_M = 1
APPVERSION_N = 0
APPVERSION_P = 8
APPVERSION   = "$(APPVERSION_M).$(APPVERSION_N).$(APPVERSION_P)"

###########################
# Set Chain environnement #
###########################

ifeq ($(CHAIN),)
CHAIN=everscale
endif

SUPPORTED_CHAINS=$(shell find makefile_conf/chain/ -type f -name '*.mk'| sed 's/.*\/\(.*\).mk/\1/g' | sort)

# Check if chain is available
ifeq ($(shell test -s ./makefile_conf/chain/$(CHAIN).mk && echo -n yes), yes)
include ./makefile_conf/chain/$(CHAIN).mk
else
$(error Unsupported CHAIN - use $(SUPPORTED_CHAINS))
endif

#########
# Other #
#########

#prepare hsm generation
ifeq ($(TARGET_NAME),TARGET_NANOS)
ICONNAME=icons/nanos_app_$(CHAIN).gif
else
ICONNAME=icons/nanox_app_$(CHAIN).gif
endif

################
# Default rule #
################
all: default

DEFINES += $(DEFINES_LIB)
DEFINES += APPNAME=\"$(APPNAME)\"
DEFINES += APPVERSION=\"$(APPVERSION)\"
DEFINES += LEDGER_MAJOR_VERSION=$(APPVERSION_M) LEDGER_MINOR_VERSION=$(APPVERSION_N) LEDGER_PATCH_VERSION=$(APPVERSION_P)
DEFINES += OS_IO_SEPROXYHAL
DEFINES += HAVE_BAGL HAVE_UX_FLOW HAVE_SPRINTF
DEFINES += HAVE_IO_USB HAVE_L4_USBLIB IO_USB_MAX_ENDPOINTS=6 IO_HID_EP_LENGTH=64 HAVE_USB_APDU
DEFINES += USB_SEGMENT_SIZE=64
DEFINES += BLE_SEGMENT_SIZE=32
DEFINES += HAVE_WEBUSB WEBUSB_URL_SIZE_B=0 WEBUSB_URL=""
DEFINES += UNUSED\(x\)=\(void\)x

ifeq ($(TARGET_NAME),TARGET_NANOX)
    DEFINES += HAVE_BLE BLE_COMMAND_TIMEOUT_MS=2000 HAVE_BLE_APDU
endif

ifeq ($(TARGET_NAME),TARGET_NANOS)
    DEFINES += IO_SEPROXYHAL_BUFFER_SIZE_B=128
else
    DEFINES += IO_SEPROXYHAL_BUFFER_SIZE_B=300
    DEFINES += HAVE_GLO096
    DEFINES += BAGL_WIDTH=128 BAGL_HEIGHT=64
    DEFINES += HAVE_BAGL_ELLIPSIS
    DEFINES += HAVE_BAGL_FONT_OPEN_SANS_REGULAR_11PX
    DEFINES += HAVE_BAGL_FONT_OPEN_SANS_EXTRABOLD_11PX
    DEFINES += HAVE_BAGL_FONT_OPEN_SANS_LIGHT_16PX
endif

DEBUG = 0
ifneq ($(DEBUG),0)
    DEFINES += HAVE_PRINTF
    ifeq ($(TARGET_NAME),TARGET_NANOS)
        DEFINES += PRINTF=screen_printf
    else
        DEFINES += PRINTF=mcu_usb_printf
    endif
else
        DEFINES += PRINTF\(...\)=
endif

##############
#  Compiler  #
##############
ifneq ($(BOLOS_ENV),)
$(info BOLOS_ENV=$(BOLOS_ENV))
CLANGPATH := $(BOLOS_ENV)/clang-arm-fropi/bin/
GCCPATH   := $(BOLOS_ENV)/gcc-arm-none-eabi-5_3-2016q1/bin/
else
$(info BOLOS_ENV is not set: falling back to CLANGPATH and GCCPATH)
endif
ifeq ($(CLANGPATH),)
$(info CLANGPATH is not set: clang will be used from PATH)
endif
ifeq ($(GCCPATH),)
$(info GCCPATH is not set: arm-none-eabi-* will be used from PATH)
endif

CC      := $(CLANGPATH)clang
CFLAGS  += -O3 -Os
AS      := $(GCCPATH)arm-none-eabi-gcc
LD      := $(GCCPATH)arm-none-eabi-gcc
LDFLAGS += -O3 -Os
LDLIBS  += -lm -lgcc -lc

# import rules to compile glyphs(/pone)
include $(BOLOS_SDK)/Makefile.glyphs

### variables processed by the common makefile.rules of the SDK to grab source files and include dirs
APP_SOURCE_PATH += src
SDK_SOURCE_PATH += lib_stusb lib_stusb_impl lib_ux

ifeq ($(TARGET_NAME),TARGET_NANOX)
    SDK_SOURCE_PATH += lib_blewbxx lib_blewbxx_impl
endif

WITH_U2F=0
ifneq ($(WITH_U2F),0)
    DEFINES         += HAVE_U2F HAVE_IO_U2F
    DEFINES         += U2F_PROXY_MAGIC=\"~$(COIN)\"
		SDK_SOURCE_PATH += lib_u2f
endif

load: all load-only
load-only:
	python3 -m ledgerblue.loadApp $(APP_LOAD_PARAMS)

load-offline: all
	python3 -m ledgerblue.loadApp $(APP_LOAD_PARAMS) --offline

delete:
	python3 -m ledgerblue.deleteApp $(COMMON_DELETE_PARAMS)

# import generic rules from the sdk
include $(BOLOS_SDK)/Makefile.rules

#add dependency on custom makefile filename
dep/%.d: %.c Makefile

listvariants:
	@echo VARIANTS CHAIN $(SUPPORTED_CHAINS)
