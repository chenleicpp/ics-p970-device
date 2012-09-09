USE_CAMERA_STUB := true

# inherit from the proprietary version
-include vendor/lge/p970/BoardConfigVendor.mk

TARGET_BOARD_PLATFORM := omap3

# Board configuration
TARGET_NO_BOOTLOADER := true
TARGET_CPU_ABI := armeabi-v7a
TARGET_CPU_ABI2 := armeabi
TARGET_ARCH_VARIANT := armv7-a-neon
ARCH_ARM_HAVE_TLS_REGISTER := true
TARGET_PROVIDES_INIT_RC := true
TARGET_PROVIDES_INIT_TARGET_RC := true
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

BOARD_HAVE_BLUETOOTH := true
BOARD_HAVE_BLUETOOTH_BCM := true

# Touchscreen
BOARD_USE_LEGACY_TOUCHSCREEN := true

# USB
BOARD_USE_USB_MASS_STORAGE_SWITCH := true
BOARD_MTP_DEVICE := "/dev/mtp"
TARGET_USE_CUSTOM_LUN_FILE_PATH := "/sys/devices/platform/usb_mass_storage/lun0/file"

# Audio
BOARD_USES_GENERIC_AUDIO := false
BOARD_USES_AUDIO_LEGACY := true
TARGET_PROVIDES_LIBAUDIO := false

# Camera
BOARD_CAMERA_LIBRARIES := libcamera
COMMON_GLOBAL_CFLAGS += -DBINDER_COMPAT

HARDWARE_OMX := true
ifdef HARDWARE_OMX
OMX_JPEG := true
OMX_VENDOR := ti
OMX_VENDOR_INCLUDES := \
   hardware/ti/omap3/omx/system/src/openmax_il/omx_core/inc
OMX_VENDOR_WRAPPER := TI_OMX_Wrapper
BOARD_OPENCORE_LIBRARIES := libOMX_Core
BOARD_OPENCORE_FLAGS := -DHARDWARE_OMX=1
endif

BOARD_MOBILEDATA_INTERFACE_NAME := "vsnet0"

BOARD_WLAN_DEVICE := bcm4329
WIFI_DRIVER_FW_STA_PATH         := "/system/etc/wifi/fw_bcm4329.bin"
WIFI_DRIVER_FW_AP_PATH          := "/system/etc/wifi/fw_bcm4329_ap.bin"
WIFI_DRIVER_MODULE_NAME         := "wireless"
WIFI_DRIVER_MODULE_PATH         := "/system/lib/modules/wireless.ko"
WIFI_DRIVER_MODULE_ARG          := "firmware_path=/system/etc/wifi/fw_bcm4329.bin nvram_path=/system/etc/wifi/nvram.txt config_path=/data/misc/wifi/config"
WPA_SUPPLICANT_VERSION          := VER_0_6_X
WIFI_DRIVER_HAS_LGE_SOFTAP      := true
BOARD_WPA_SUPPLICANT_DRIVER := WEXT
BOARD_WEXT_NO_COMBO_SCAN	:= true

BOARD_EGL_CFG := device/lge/p970/configs/egl.cfg
COMMON_GLOBAL_CFLAGS += -DSURFACEFLINGER_FORCE_SCREEN_RELEASE
USE_OPENGL_RENDERER := true

TARGET_USERIMAGES_USE_EXT4 := true
BOARD_SYSTEMIMAGE_PARTITION_SIZE := 665681920
BOARD_USERDATAIMAGE_PARTITION_SIZE := 1170259968
BOARD_FLASH_BLOCK_SIZE := 131072

COMMON_GLOBAL_CFLAGS += -DOVERLAY_NUM_REQBUFFERS=6


BOARD_GLOBAL_CFLAGS += -DCHARGERMODE_CMDLINE_NAME='"rs"' -DCHARGERMODE_CMDLINE_VALUE='"c"'
