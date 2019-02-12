################################################################################
#    IconX App
#    (c) 2018 TheLoop
################################################################################
ifeq ($(BOLOS_SDK),)
$(error Environment variable BOLOS_SDK is not set)
endif
include $(BOLOS_SDK)/Makefile.defines


# All but bitcoin app use dependency onto the bitcoin app/lib
APP_LOAD_FLAGS=--appFlags 0x50
APP_LOAD_PARAMS=--curve secp256k1 $(COMMON_LOAD_PARAMS) 

APPVERSION_M=1
APPVERSION_N=0
APPVERSION_P=3
APPVERSION=$(APPVERSION_M).$(APPVERSION_N).$(APPVERSION_P)

APP_LOAD_PARAMS += $(APP_LOAD_FLAGS)

DEFINES += $(DEFINES_LIB)

ifeq ($(COIN),)
COIN=icon
endif

ifeq ($(TARGET_NAME),TARGET_BLUE)
ICONNAME=blue_app_$(COIN).gif
else
ICONNAME=nanos_app_$(COIN).gif
endif


ifeq ($(COIN),icon)
APP_PATH = "44'/4801368'"
APPNAME=ICON
else ifeq ($(COIN),icon_testnet)
APP_PATH = "44'/1'"
APPNAME="ICON testnet"
else
ifeq ($(filter clean,$(MAKECMDGOALS)),)
$(error Unsupported COIN - use icon, icon_testnet)
endif
endif

APP_LOAD_PARAMS += --path $(APP_PATH)



################
# Default rule #
################

all: default

############
# Platform #
############

DEFINES   += OS_IO_SEPROXYHAL IO_SEPROXYHAL_BUFFER_SIZE_B=300
DEFINES   += HAVE_BAGL HAVE_SPRINTF
#DEFINES   += HAVE_PRINTF PRINTF=screen_printf
DEFINES   += PRINTF\(...\)=
DEFINES   += HAVE_IO_USB HAVE_L4_USBLIB IO_USB_MAX_ENDPOINTS=6 IO_HID_EP_LENGTH=64 HAVE_USB_APDU
DEFINES   += LEDGER_MAJOR_VERSION=$(APPVERSION_M) LEDGER_MINOR_VERSION=$(APPVERSION_N) LEDGER_PATCH_VERSION=$(APPVERSION_P) TCS_LOADER_PATCH_VERSION=0
DEFINES   += UNUSED\(x\)=\(void\)x
DEFINES   += APPVERSION=\"$(APPVERSION)\"

# We don't need compliance with 141 version of cx_xxx APIs.
#DEFINES += CX_COMPLIANCE_141

CC       := $(CLANGPATH)clang 
#CFLAGS   += -O0
CFLAGS   += -O3 -Os

AS     := $(GCCPATH)arm-none-eabi-gcc

LD       := $(GCCPATH)arm-none-eabi-gcc
LDFLAGS  += -O3 -Os
LDLIBS   += -lm -lgcc -lc 

# import rules to compile glyphs(/pone)
#include $(BOLOS_SDK)/Makefile.glyphs

### variables processed by the common makefile.rules of the SDK to grab source files and include dirs
APP_SOURCE_PATH  += src
SDK_SOURCE_PATH  += lib_stusb
SDK_SOURCE_PATH  += lib_u2f lib_stusb_impl

DEFINES   += USB_SEGMENT_SIZE=64 
DEFINES   += U2F_PROXY_MAGIC=\"ICON\"
DEFINES   += HAVE_IO_U2F HAVE_U2F 

load: all
	python -m ledgerblue.loadApp $(APP_LOAD_PARAMS)

delete:
	python -m ledgerblue.deleteApp $(COMMON_DELETE_PARAMS)

# import generic rules from the sdk
include $(BOLOS_SDK)/Makefile.rules

#add dependency on custom makefile filename
dep/%.d: %.c Makefile

listvariants:
	@echo VARIANTS COIN icon icon_testnet
