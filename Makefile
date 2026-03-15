# Project Name
TARGET = ElianeDSP

# Build for DFU flashing via CLI (changed from BOOT_SRAM to BOOT_NONE for DFU compatibility)
APP_TYPE = BOOT_NONE

# Sources — M2: Engine class with one oscillator pair + ring mod + filter
CPP_SOURCES = \
    ElianeDSP_main.cpp \
    Source/Engine.cpp

# Path to Aurora SDK (cloned as submodule)
AURORA_SDK_PATH = sdks/Aurora-SDK

# Include paths
C_INCLUDES += -I$(AURORA_SDK_PATH)/include/ -ISource/

# Library locations (bundled with Aurora SDK)
LIBDAISY_DIR = $(AURORA_SDK_PATH)/libs/libDaisy/
DAISYSP_DIR = $(AURORA_SDK_PATH)/libs/DaisySP/

# Core location, and generic Makefile
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile
