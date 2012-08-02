# Copyright (C) 2009 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

#
# This file sets variables that control the way modules are built
# thorughout the system. It should not be used to conditionally
# disable makefiles (the proper mechanism to control what gets
# included in a build is to use PRODUCT_PACKAGES in a product
# definition file).
#

# inherit from the proprietary version
-include vendor/lge/p970/BoardConfigVendor.mk

TARGET_BOARD_PLATFORM := omap3

# Board configuration
#
TARGET_NO_BOOTLOADER := true
TARGET_CPU_ABI := armeabi-v7a
TARGET_CPU_ABI2 := armeabi
TARGET_ARCH_VARIANT := armv7-a-neon
ARCH_ARM_HAVE_TLS_REGISTER := true
TARGET_PROVIDES_INIT_RC := true
OMAP_ENHANCEMENT := true

TARGET_BOOTLOADER_BOARD_NAME := p970

BOARD_KERNEL_CMDLINE := 
BOARD_KERNEL_BASE := 0x80000000
BOARD_PAGE_SIZE := 0x00000800

ifdef OMAP_ENHANCEMENT
COMMON_GLOBAL_CFLAGS += -DOMAP_ENHANCEMENT -DTARGET_OMAP3
endif

TARGET_PREBUILT_KERNEL := device/lge/p970/kernel

BOARD_NEEDS_CUTILS_LOG := true

BOARD_HAS_NO_SELECT_BUTTON := true
# Use this flag if the board has a ext4 partition larger than 2gb
#BOARD_HAS_LARGE_FILESYSTEM := true

BOARD_HAVE_BLUETOOTH := true
BOARD_HAVE_BLUETOOTH_BCM := true

# Camera
USE_CAMERA_STUB := false
BOARD_USES_TI_CAMERA_HAL := true

# FM Radio (jordan said it's not ready in ICS)
#BOARD_HAVE_FM_RADIO := true
#COMMON_GLOBAL_CFLAGS += -DHAVE_FM_RADIO

# Touchscreen
BOARD_USE_LEGACY_TOUCHSCREEN := true

# USB
BOARD_USE_USB_MASS_STORAGE_SWITCH := true
BOARD_MASS_STORAGE_FILE_PATH := "/sys/devices/platform/usb_mass_storage/lun0/file"
BOARD_MTP_DEVICE := "/dev/mtp"

# Audio
BOARD_USES_ALSA_AUDIO := true
BOARD_USES_GENERIC_AUDIO := false
BUILD_WITH_ALSA_UTILS := true
BOARD_USES_TI_OMAP_MODEM_AUDIO := true

# add audio legacy
BOARD_USES_AUDIO_LEGACY := true
TARGET_PROVIDES_LIBAUDIO := true

# OMX
HARDWARE_OMX := true
ifdef HARDWARE_OMX
OMX_JPEG := true
OMX_VENDOR := ti
OMX_VENDOR_INCLUDES := \
   hardware/ti/omap3/omx/system/src/openmax_il/omx_core/inc
OMX_VENDOR_WRAPPER := TI_OMX_Wrapper
BOARD_OPENCORE_LIBRARIES := libOMX_Core
BOARD_OPENCORE_FLAGS := -DHARDWARE_OMX=1
#BOARD_CAMERA_LIBRARIES := libcamera
endif

BOARD_MOBILEDATA_INTERFACE_NAME := "vsnet0"

BOARD_WLAN_DEVICE               := bcm4329
WIFI_DRIVER_FW_STA_PATH         := "/system/etc/wifi/fw_bcm4329.bin"
WIFI_DRIVER_FW_AP_PATH          := "/system/etc/wifi/fw_bcm4329_ap.bin"
WIFI_DRIVER_MODULE_NAME         := "wireless"
WIFI_DRIVER_MODULE_PATH         := "/system/lib/modules/wireless.ko"
WIFI_DRIVER_MODULE_ARG          := "firmware_path=/system/etc/wifi/fw_bcm4329_p2p.bin nvram_path=/system/etc/wifi/nvram.txt config_path=/data/misc/wifi/config dhd_use_p2p=1"
WPA_SUPPLICANT_VERSION          := VER_0_6_X
WIFI_DRIVER_HAS_LGE_SOFTAP      := true
BOARD_WPA_SUPPLICANT_DRIVER := WEXT
BOARD_WEXT_NO_COMBO_SCAN	:= true

BOARD_EGL_CFG := device/lge/p970/config/egl.cfg
COMMON_GLOBAL_CFLAGS += -DSURFACEFLINGER_FORCE_SCREEN_RELEASE
USE_OPENGL_RENDERER := true

TARGET_USERIMAGES_USE_EXT4 := true
BOARD_SYSTEMIMAGE_PARTITION_SIZE := 665681920
BOARD_USERDATAIMAGE_PARTITION_SIZE := 1170259968
BOARD_FLASH_BLOCK_SIZE := 131072

BOARD_OMAP3_WITH_FFC := true
BOARD_HAS_OMAP3_FW3A_LIBCAMERA := true

# Overwrite number of overlay buffers
COMMON_GLOBAL_CFLAGS += -DOVERLAY_NUM_REQBUFFERS=6

BOARD_GLOBAL_CFLAGS += -DCHARGERMODE_CMDLINE_NAME='"rs"' -DCHARGERMODE_CMDLINE_VALUE='"c"'
