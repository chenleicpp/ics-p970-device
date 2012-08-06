#!/bin/sh

# Copyright (C) 2011 The CyanogenMod Project
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

VENDOR=lge
DEVICE=p970

mkdir -p ../../../vendor/$VENDOR/$DEVICE

(cat << EOF) | sed s/__DEVICE__/$DEVICE/g | sed s/__VENDOR__/$VENDOR/g > ../../../vendor/$VENDOR/$DEVICE/$DEVICE-vendor.mk
# Copyright (C) 2011 The CyanogenMod Project
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

# This file is generated by device/__VENDOR__/__DEVICE__/setup-makefiles.sh

# Live wallpaper packages
PRODUCT_PACKAGES := \\
    LiveWallpapers \\
    LiveWallpapersPicker \\
    MagicSmokeWallpapers \\
    VisualizationWallpapers \\
    librs_jni

# Publish that we support the live wallpaper feature.
PRODUCT_COPY_FILES := \\
    packages/wallpapers/LivePicker/android.software.live_wallpaper.xml:/system/etc/permissions/android.software.live_wallpaper.xml

\$(call inherit-product, vendor/__VENDOR__/__DEVICE__/__DEVICE__-vendor-blobs.mk)
EOF

(cat << EOF) | sed s/__DEVICE__/$DEVICE/g | sed s/__VENDOR__/$VENDOR/g > ../../../vendor/$VENDOR/$DEVICE/$DEVICE-vendor-blobs.mk
# Copyright (C) 2011 The CyanogenMod Project
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

# This file is generated by device/__VENDOR__/__DEVICE__/setup-makefiles.sh

# Prebuilt
PRODUCT_COPY_FILES := \\
	vendor/__VENDOR__/__DEVICE__/proprietary/lib/libril.so:obj/lib/libril.so \\
	vendor/__VENDOR__/__DEVICE__/proprietary/lib/libaudio.so:obj/lib/libaudio.so \\
	vendor/__VENDOR__/__DEVICE__/proprietary/lib/libcamera.so:obj/lib/libcamera.so
	
# HAL
PRODUCT_COPY_FILES += \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/hw/gralloc.omap3.so:system/lib/hw/gralloc.omap3.so \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/hw/lights.omap3.so:system/lib/hw/lights.omap3.so \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/hw/sensors.omap3.so:system/lib/hw/sensors.omap3.so
    
# Audio
PRODUCT_COPY_FILES += \\
	vendor/__VENDOR__/__DEVICE__/proprietary/lib/hw/alsa.p970.so:system/lib/hw/alsa.p970.so \\
	vendor/__VENDOR__/__DEVICE__/proprietary/lib/hw/audio.primary.omap3.so:system/lib/hw/audio.primary.omap3.so \\
	vendor/__VENDOR__/__DEVICE__/proprietary/lib/liba2dp.so:system/lib/liba2dp.so \\
	vendor/__VENDOR__/__DEVICE__/proprietary/lib/libasound.so:system/lib/libasound.so \\
	vendor/__VENDOR__/__DEVICE__/proprietary/lib/libaudio.so:system/lib/libaudio.so \\
	vendor/__VENDOR__/__DEVICE__/proprietary/lib/libaudiomodemgeneric.so:system/lib/libaudiomodemgeneric.so

# PVRSGX
PRODUCT_COPY_FILES += \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/egl/libEGL_POWERVR_SGX530_125.so:system/lib/egl/libEGL_POWERVR_SGX530_125.so \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/egl/libGLESv1_CM_POWERVR_SGX530_125.so:system/lib/egl/libGLESv1_CM_POWERVR_SGX530_125.so \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/egl/libGLESv2_POWERVR_SGX530_125.so:system/lib/egl/libGLESv2_POWERVR_SGX530_125.so \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/libsrv_um.so:system/lib/libsrv_um.so \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/libsrv_init.so:system/lib/libsrv_init.so \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/libpvr2d.so:system/lib/libpvr2d.so \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/libpvrANDROID_WSEGL.so:system/lib/libpvrANDROID_WSEGL.so \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/libIMGegl.so:system/lib/libIMGegl.so \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/libglslcompiler.so:system/lib/libglslcompiler.so \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/libusc.so:system/lib/libusc.so \\
    vendor/__VENDOR__/__DEVICE__/proprietary/bin/pvrsrvinit:system/bin/pvrsrvinit


# RIL
PRODUCT_COPY_FILES += \\
    vendor/__VENDOR__/__DEVICE__/proprietary/bin/rild:system/bin/rild \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/lge-ril.so:system/lib/lge-ril.so \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/libril.so:system/lib/libril.so \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/libini.so:system/lib/libini.so \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/libreference-ril.so:system/lib/libreference-ril.so

# GPS
PRODUCT_COPY_FILES += \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/hw/gps.omap3.so:system/lib/hw/gps.omap3.so \\
    vendor/__VENDOR__/__DEVICE__/proprietary/bin/glgps:system/bin/glgps

# Sensors
PRODUCT_COPY_FILES += \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/libmpl.so:system/lib/libmpl.so \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/libmllite.so:system/lib/libmllite.so \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/libmlplatform.so:system/lib/libmlplatform.so

# Wifi
PRODUCT_COPY_FILES += \\
    vendor/__VENDOR__/__DEVICE__/proprietary/etc/wifi/fw_bcm4329.bin:system/etc/wifi/fw_bcm4329.bin \\
    vendor/__VENDOR__/__DEVICE__/proprietary/etc/wifi/fw_bcm4329_ap.bin:system/etc/wifi/fw_bcm4329_ap.bin

# DSP
PRODUCT_COPY_FILES += \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/libaffw_2.0.so:system/lib/libaffw_2.0.so \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/libaf_lg_2.0.so:system/lib/libaf_lg_2.0.so \\
    vendor/__VENDOR__/__DEVICE__/proprietary/bin/fw3a_core:system/bin/fw3a_core \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/dsp/720p_h264vdec_sn.dll64P:system/lib/dsp/720p_h264vdec_sn.dll64P \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/dsp/720p_h264venc_sn.dll64P:system/lib/dsp/720p_h264venc_sn.dll64P \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/dsp/720p_mp4vdec_sn.dll64P:system/lib/dsp/720p_mp4vdec_sn.dll64P \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/dsp/720p_mp4venc_sn.dll64P:system/lib/dsp/720p_mp4venc_sn.dll64P \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/dsp/720p_wmv9vdec_sn.dll64P:system/lib/dsp/720p_wmv9vdec_sn.dll64P \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/dsp/baseimage.dof:system/lib/dsp/baseimage.dof \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/dsp/baseimage.map:system/lib/dsp/baseimage.map \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/dsp/chromasuppress.l64p:system/lib/dsp/chromasuppress.l64p \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/dsp/conversions.dll64P:system/lib/dsp/conversions.dll64P \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/dsp/dctn_dyn.dll64P:system/lib/dsp/dctn_dyn.dll64P \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/dsp/ddspbase_tiomap3430.dof64P:system/lib/dsp/ddspbase_tiomap3430.dof64P \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/dsp/dfgm.dll64P:system/lib/dsp/dfgm.dll64P \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/dsp/dynbase_tiomap3430.dof64P:system/lib/dsp/dynbase_tiomap3430.dof64P \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/dsp/eenf_ti.l64P:system/lib/dsp/eenf_ti.l64P \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/dsp/g711dec_sn.dll64P:system/lib/dsp/g711dec_sn.dll64P \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/dsp/g711enc_sn.dll64P:system/lib/dsp/g711enc_sn.dll64P \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/dsp/g722dec_sn.dll64P:system/lib/dsp/g722dec_sn.dll64P \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/dsp/g722enc_sn.dll64P:system/lib/dsp/g722enc_sn.dll64P \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/dsp/g726dec_sn.dll64P:system/lib/dsp/g726dec_sn.dll64P \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/dsp/g726enc_sn.dll64P:system/lib/dsp/g726enc_sn.dll64P \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/dsp/g729dec_sn.dll64P:system/lib/dsp/g729dec_sn.dll64P \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/dsp/g729enc_sn.dll64P:system/lib/dsp/g729enc_sn.dll64P \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/dsp/h264vdec_sn.dll64P:system/lib/dsp/h264vdec_sn.dll64P \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/dsp/h264venc_sn.dll64P:system/lib/dsp/h264venc_sn.dll64P \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/dsp/ilbcdec_sn.dll64P:system/lib/dsp/ilbcdec_sn.dll64P \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/dsp/ilbcenc_sn.dll64P:system/lib/dsp/ilbcenc_sn.dll64P \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/dsp/ipp_sn.dll64P:system/lib/dsp/ipp_sn.dll64P \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/dsp/jpegdec_sn.dll64P:system/lib/dsp/jpegdec_sn.dll64P \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/dsp/jpegenc_sn.dll64P:system/lib/dsp/jpegenc_sn.dll64P \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/dsp/m4venc_sn.dll64P:system/lib/dsp/m4venc_sn.dll64P \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/dsp/monitor_tiomap3430.dof64P:system/lib/dsp/monitor_tiomap3430.dof64P \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/dsp/mp3dec_sn.dll64P:system/lib/dsp/mp3dec_sn.dll64P \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/dsp/mp4vdec_sn.dll64P:system/lib/dsp/mp4vdec_sn.dll64P \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/dsp/mpeg4aacdec_sn.dll64P:system/lib/dsp/mpeg4aacdec_sn.dll64P \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/dsp/mpeg4aacenc_sn.dll64P:system/lib/dsp/mpeg4aacenc_sn.dll64P \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/dsp/nbamrdec_sn.dll64P:system/lib/dsp/nbamrdec_sn.dll64P \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/dsp/nbamrenc_sn.dll64P:system/lib/dsp/nbamrenc_sn.dll64P \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/dsp/postprocessor_dualout.dll64P:system/lib/dsp/postprocessor_dualout.dll64P \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/dsp/qosdyn_3430.dll64P:system/lib/dsp/qosdyn_3430.dll64P \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/dsp/ringio.dll64P:system/lib/dsp/ringio.dll64P \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/dsp/sparkdec_sn.dll64P:system/lib/dsp/sparkdec_sn.dll64P \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/dsp/star.l64P:system/lib/dsp/star.l64P \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/dsp/usn.dll64P:system/lib/dsp/usn.dll64P \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/dsp/vpp_sn.dll64P:system/lib/dsp/vpp_sn.dll64P \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/dsp/wbamrdec_sn.dll64P:system/lib/dsp/wbamrdec_sn.dll64P \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/dsp/wbamrenc_sn.dll64P:system/lib/dsp/wbamrenc_sn.dll64P \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/dsp/wmadec_sn.dll64P:system/lib/dsp/wmadec_sn.dll64P \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/dsp/wmv9dec_sn.dll64P:system/lib/dsp/wmv9dec_sn.dll64P \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/dsp/yuvconvert.l64p:system/lib/dsp/yuvconvert.l64p

# BT Firmware
PRODUCT_COPY_FILES += \\
    vendor/__VENDOR__/__DEVICE__/proprietary/etc/firmware/BCM43291A0_003.001.013.0066.xxxx_B-Project.hcd:system/etc/firmware/BCM43291A0_003.001.013.0066.xxxx_B-Project.hcd

# Camera and related blobs
PRODUCT_COPY_FILES += \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/libyuvfastconvert.so:system/lib/libyuvfastconvert.so \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/libicapture.so:system/lib/libicapture.so \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/libicamera.so:system/lib/libicamera.so \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/libcapl.so:system/lib/libcapl.so \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/libcameraalgo.so:system/lib/libcameraalgo.so \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/libcamera.so:system/lib/libcamera.so \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/libarcsoft_camera_func.so:system/lib/libarcsoft_camera_func.so \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/libImagePipeline.so:system/lib/libImagePipeline.so \\
    vendor/__VENDOR__/__DEVICE__/proprietary/etc/omapcam/imx072_dtp.dat:system/etc/omapcam/imx072_dtp.dat \\
    vendor/__VENDOR__/__DEVICE__/proprietary/etc/omapcam/imx072.rev:system/etc/omapcam/imx072.rev \\
    vendor/__VENDOR__/__DEVICE__/proprietary/etc/omapcam/fw3a.conf:system/etc/omapcam/fw3a.conf
# OMX 720p libraries
PRODUCT_COPY_FILES += \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/libOMX.TI.mp4.splt.Encoder.so:system/lib/libOMX.TI.mp4.splt.Encoder.so \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/libOMX.TI.720P.Encoder.so:system/lib/libOMX.TI.720P.Encoder.so \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/libOMX.TI.720P.Decoder.so:system/lib/libOMX.TI.720P.Decoder.so \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/libOMX.ITTIAM.AAC.encode.so:system/lib/libOMX.ITTIAM.AAC.encode.so \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/libOMX.ITTIAM.AAC.decode.so:system/lib/libOMX.ITTIAM.AAC.decode.so \\
    vendor/__VENDOR__/__DEVICE__/proprietary/lib/libOMX.TI.h264.splt.Encoder.so:system/lib/libOMX.TI.h264.splt.Encoder.so

EOF

(cat << EOF) | sed s/__DEVICE__/$DEVICE/g | sed s/__VENDOR__/$VENDOR/g > ../../../vendor/$VENDOR/$DEVICE/BoardConfigVendor.mk
# Copyright (C) 2011 The CyanogenMod Project
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

# This file is generated by device/__VENDOR__/__DEVICE__/setup-makefiles.sh

EOF

