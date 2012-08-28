/*
 * Copyright (C) 2012 ShenduOS Team
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
 
#define LOG_TAG "***CameraHAL***"
#define LOG_NDEBUG 0

#include "CameraHardwareInterface.h"
#include <hardware/hardware.h>
#include <hardware/camera.h>
#include <binder/IMemory.h>
#include <camera/CameraParameters.h>
#include <fcntl.h>
#include <linux/ioctl.h>
#include <hardware/gralloc.h>

using android::sp;
using android::Overlay;
using android::String8;
using android::IMemory;
using android::IMemoryHeap;
using android::CameraParameters;
using android::CameraInfo;
using android::HAL_getCameraInfo;
using android::HAL_getNumberOfCameras;
using android::HAL_openCameraHardware;
using android::CameraHardwareInterface;

#define LOG_FUNCTION_NAME           LOGD("%d: %s() ENTER", __LINE__, __FUNCTION__);
#define NO_ERROR 0

extern "C" android::sp<android::CameraHardwareInterface> HAL_openCameraHardware(int cameraId);
extern "C" int HAL_getNumberOfCameras();
extern "C" void HAL_getCameraInfo(int cameraId, struct CameraInfo* cameraInfo);

int camera_device_open(const hw_module_t* module, const char* name,
                        hw_device_t** device);
int camera_get_camera_info(int camera_id, struct camera_info *info);
int camera_get_number_of_cameras(void);

struct legacy_camera_device {
   camera_device_t device;
   android::sp<android::CameraHardwareInterface> hwif;
   int id;

   camera_notify_callback         notify_callback;
   camera_data_callback           data_callback;
   camera_data_timestamp_callback data_timestamp_callback;
   camera_request_memory          request_memory;
   void                          *user;

   preview_stream_ops *window;
   gralloc_module_t const *gralloc;
   camera_memory_t *clientData;
};

static struct {
    int type;
    const char *text;
} msg_map[] = {
    {0x0001, "CAMERA_MSG_ERROR"},
    {0x0002, "CAMERA_MSG_SHUTTER"},
    {0x0004, "CAMERA_MSG_FOCUS"},
    {0x0008, "CAMERA_MSG_ZOOM"},
    {0x0010, "CAMERA_MSG_PREVIEW_FRAME"},
    {0x0020, "CAMERA_MSG_VIDEO_FRAME"},
    {0x0040, "CAMERA_MSG_POSTVIEW_FRAME"},
    {0x0080, "CAMERA_MSG_RAW_IMAGE"},
    {0x0100, "CAMERA_MSG_COMPRESSED_IMAGE"},
    {0x0200, "CAMERA_MSG_RAW_IMAGE_NOTIFY"},
    {0x0400, "CAMERA_MSG_PREVIEW_METADATA"},
    {0x0000, "CAMERA_MSG_ALL_MSGS"}, //0xFFFF
    {0x0000, "NULL"},
};

static void dump_msg(const char *tag, int msg_type)
{
    int i;
    for (i = 0; msg_map[i].type; i++) {
        if (msg_type & msg_map[i].type) {
            LOGD("%s: %s", tag, msg_map[i].text);
        }
    }
}

static hw_module_methods_t camera_module_methods = {
   open: camera_device_open
};

camera_module_t HAL_MODULE_INFO_SYM = {
   common: {
      tag: HARDWARE_MODULE_TAG,
      version_major: 1,
      version_minor: 0,
      id: CAMERA_HARDWARE_MODULE_ID,
      name: "Camera HAL for P970 ICS",
      author: "ChenLei",
      methods: &camera_module_methods,
      dso: NULL,
      reserved: {0},
   },
   get_number_of_cameras: camera_get_number_of_cameras,
   get_camera_info: camera_get_camera_info,
};

static inline struct legacy_camera_device * to_lcdev(struct camera_device *dev)
{
    return reinterpret_cast<struct legacy_camera_device *>(dev);
}

camera_memory_t * wrap_memory_data(const android::sp<android::IMemory> &dataPtr,
                        void *user)
{
   LOG_FUNCTION_NAME
   ssize_t          offset;
   size_t           size;
   camera_memory_t *clientData = NULL;
   struct legacy_camera_device *lcdev = (struct legacy_camera_device *) user;
   android::sp<android::IMemoryHeap> mHeap = dataPtr->getMemory(&offset, &size);

   clientData = lcdev->request_memory(-1, size, 1, lcdev->user);
   if (clientData != NULL) {
      memcpy(clientData->data, (char *)(mHeap->base()) + offset, size);
   } else {
      LOGE("wrap_memory_data: ERROR allocating memory from client\n");
   }
   return clientData;
}

void wrap_notify_callback(int32_t msg_type, int32_t ext1,
                   int32_t ext2, void *user)
{
   LOG_FUNCTION_NAME
   struct legacy_camera_device *lcdev = (struct legacy_camera_device *) user;
   LOGD("%s: msg_type:%d ext1:%d ext2:%d user:%p\n",__FUNCTION__,
        msg_type, ext1, ext2, user);
   dump_msg(__FUNCTION__, msg_type);
   if (lcdev->notify_callback != NULL) {
      lcdev->notify_callback(msg_type, ext1, ext2, lcdev->user);
   }
}

void wrap_data_callback(int32_t msg_type, const android::sp<android::IMemory>& dataPtr,
                 void *user)
{
   LOG_FUNCTION_NAME
   struct legacy_camera_device *lcdev = (struct legacy_camera_device *) user;

   LOGD("%s: msg_type:%d user:%p\n",__FUNCTION__,msg_type, user);
   dump_msg(__FUNCTION__, msg_type);

   if (lcdev->data_callback != NULL && lcdev->request_memory != NULL) {
      /* Make sure any pre-existing heap is released */
      if (lcdev->clientData != NULL) {
         lcdev->clientData->release(lcdev->clientData);
         lcdev->clientData = NULL;
      }
      lcdev->clientData = wrap_memory_data(dataPtr, lcdev);
      if (lcdev->clientData != NULL) {
         //LOGV("CameraHAL_DataCb: Posting data to client\n");
         lcdev->data_callback(msg_type, lcdev->clientData, 0, NULL, lcdev->user);
      }
   }
#if 0
   if (msg_type == CAMERA_MSG_PREVIEW_FRAME) {
      int32_t previewWidth, previewHeight;
      android::CameraParameters hwParameters(lcdev->hwif->getParameters());
      hwParameters.getPreviewSize(&previewWidth, &previewHeight);
      LOGV("%s: preview size = %dx%d\n",__FUNCTION__, previewWidth, previewHeight);
      CameraHAL_HandlePreviewData(dataPtr, previewWidth, previewHeight, lcdev);
   }
#else
#endif
}

void wrap_data_callback_timestamp(nsecs_t timestamp, int32_t msg_type,
                   const android::sp<android::IMemory>& dataPtr, void *user)
{
   LOG_FUNCTION_NAME
   struct legacy_camera_device *lcdev = (struct legacy_camera_device *) user;

   LOGD("%s: timestamp:%lld msg_type:%d user:%p\n",__FUNCTION__,
        timestamp /1000, msg_type, user);
   dump_msg(__FUNCTION__, msg_type);

   if (lcdev->data_callback != NULL && lcdev->request_memory != NULL) {
      camera_memory_t *clientData = wrap_memory_data(dataPtr, lcdev);
      if (clientData != NULL) {
         LOGD("wrap_data_callback_timestamp: Posting data to client timestamp:%lld\n",
              systemTime());
         lcdev->data_timestamp_callback(timestamp, msg_type, clientData, 0, lcdev->user);
         lcdev->hwif->releaseRecordingFrame(dataPtr);
      } else {
         LOGD("wrap_data_callback_timestamp: ERROR allocating memory from client\n");
      }
   }
}

/*******************************************************************
 * implementation of camera_device_ops_t functions
 *******************************************************************/

 void camera_fixup_params(android::CameraParameters &camParams)
{
   const char *preferred_size = "640x480";
  
   if (!camParams.get(android::CameraParameters::KEY_VIDEO_FRAME_FORMAT))
   {
      LOGE("%s:parame1:%s",__FUNCTION__,camParams.get(android::CameraParameters::KEY_VIDEO_FRAME_FORMAT));
      camParams.set(android::CameraParameters::KEY_VIDEO_FRAME_FORMAT,
                  android::CameraParameters::PIXEL_FORMAT_YUV420SP);
      LOGE("%s:parame1:%s",__FUNCTION__,camParams.get(android::CameraParameters::KEY_VIDEO_FRAME_FORMAT));
   }
    
   if (!camParams.get(android::CameraParameters::KEY_PREFERRED_PREVIEW_SIZE_FOR_VIDEO))
   {
      LOGE("%s:parame2:%s",__FUNCTION__,camParams.get(android::CameraParameters::KEY_PREFERRED_PREVIEW_SIZE_FOR_VIDEO));
      camParams.set(CameraParameters::KEY_PREFERRED_PREVIEW_SIZE_FOR_VIDEO,
                  preferred_size);
      LOGE("%s:parame2:%s",__FUNCTION__,camParams.get(android::CameraParameters::KEY_PREFERRED_PREVIEW_SIZE_FOR_VIDEO));
   }
}

int camera_get_camera_info(int camera_id, struct camera_info *info)
{
	LOG_FUNCTION_NAME
	int rv = 0;
   	android::CameraInfo cam_info;
   	android::HAL_getCameraInfo(camera_id, &cam_info);
   	info->facing = cam_info.facing;
   	info->orientation = cam_info.orientation;
   	LOGD("%s: id:%i faceing:%i orientation: %i", __FUNCTION__,
        camera_id, info->facing, info->orientation);
   	return rv;
}

int camera_get_number_of_cameras(void)
{
	LOG_FUNCTION_NAME
	int num_cameras = HAL_getNumberOfCameras();
     LOGD("%s: number:%i", __FUNCTION__, num_cameras);
     return num_cameras;
}

int camera_set_preview_window(struct camera_device * device,
                           struct preview_stream_ops *window)
{
   LOG_FUNCTION_NAME
   	int rv = -EINVAL;
   int min_bufs = -1;
   int kBufferCount = 4;
   struct legacy_camera_device *lcdev = to_lcdev(device);

   LOGV("%s : Window :%p\n",__FUNCTION__, window);
   if (device == NULL) {
      LOGE("camera_set_preview_window : Invalid device.\n");
      return -EINVAL;
   }

   if (lcdev->window == window) {
      return 0;
   }

   lcdev->window = window;

   if (!window) {
      LOGD("%s: window is NULL", __FUNCTION__);
      return -EINVAL;
   }

   LOGV("%s : OK window is %p", __FUNCTION__, window);

   if (!lcdev->gralloc) {
      hw_module_t const* module;
      int err = 0;
      if (hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &module) == 0) {
         lcdev->gralloc = (const gralloc_module_t *)module;
      } else {
         LOGE("%s: Fail on loading gralloc HAL", __FUNCTION__);
      }
   }


   LOGV("%s: OK on loading gralloc HAL", __FUNCTION__);
   if (window->get_min_undequeued_buffer_count(window, &min_bufs)) {
      LOGE("%s: could not retrieve min undequeued buffer count", __FUNCTION__);
      return -1;
   }
   LOGV("%s: OK get_min_undequeued_buffer_count", __FUNCTION__);

   LOGV("%s: bufs:%i", __FUNCTION__, min_bufs);
   if (min_bufs >= kBufferCount) {
      LOGE("%s: min undequeued buffer count %i is too high (expecting at most %i)",
          __FUNCTION__, min_bufs, kBufferCount - 1);
   }

   LOGV("%s: setting buffer count to %i", __FUNCTION__, kBufferCount);
   if (window->set_buffer_count(window, kBufferCount)) {
      LOGE("%s: could not set buffer count", __FUNCTION__);
      return -1;
   }

   int w, h;
   android::CameraParameters params(lcdev->hwif->getParameters());
   params.getPreviewSize(&w, &h);
   int hal_pixel_format = HAL_PIXEL_FORMAT_YCrCb_420_SP;
   const char *str_preview_format = params.getPreviewFormat();
   LOGV("%s: preview format %s", __FUNCTION__, str_preview_format);

   if (window->set_usage(window, GRALLOC_USAGE_SW_WRITE_MASK)) {
      LOGE("%s: could not set usage on gralloc buffer", __FUNCTION__);
      return -1;
   }

   if (window->set_buffers_geometry(window, w, h, hal_pixel_format)) {
      LOGE("%s: could not set buffers geometry to %s",
          __FUNCTION__, str_preview_format);
      return -1;
   }

   return NO_ERROR;
}



void camera_set_callbacks(struct camera_device * device,
                      camera_notify_callback notify_cb,
                      camera_data_callback data_cb,
                      camera_data_timestamp_callback data_cb_timestamp,
                      camera_request_memory get_memory, 
                      void *user)
{
	LOG_FUNCTION_NAME
   struct legacy_camera_device *lcdev = to_lcdev(device);
   LOGV("%s: notify_cb: %p, data_cb: %p "
         "data_cb_timestamp: %p, get_memory: %p, user :%p",__FUNCTION__,
         notify_cb, data_cb, data_cb_timestamp, get_memory, user);

   lcdev->notify_callback = notify_cb;
   lcdev->data_callback = data_cb;
   lcdev->data_timestamp_callback = data_cb_timestamp;
   lcdev->request_memory = get_memory;
   lcdev->user = user;

   lcdev->hwif->setCallbacks(wrap_notify_callback, wrap_data_callback,wrap_data_callback_timestamp, (void *) lcdev);
}

void camera_enable_msg_type(struct camera_device * device, int32_t msg_type)
{
   LOG_FUNCTION_NAME
   struct legacy_camera_device *lcdev = to_lcdev(device);
   LOGV("%s: msg_type:%d\n",__FUNCTION__,msg_type);
   dump_msg(__FUNCTION__, msg_type);
   lcdev->hwif->enableMsgType(msg_type);
}

void camera_disable_msg_type(struct camera_device * device, int32_t msg_type)
{
   LOG_FUNCTION_NAME
   struct legacy_camera_device *lcdev = to_lcdev(device);
   LOGV("camera_disable_msg_type: msg_type:%d\n", msg_type);
   dump_msg(__FUNCTION__, msg_type);
   lcdev->hwif->disableMsgType(msg_type);
}

int camera_msg_type_enabled(struct camera_device * device, int32_t msg_type)
{
	LOG_FUNCTION_NAME
   struct legacy_camera_device *lcdev = to_lcdev(device);
   LOGV("camera_msg_type_enabled: msg_type:%d\n", msg_type);
   dump_msg(__FUNCTION__, msg_type);
   return lcdev->hwif->msgTypeEnabled(msg_type);
}

int camera_start_preview(struct camera_device * device)
{
	LOG_FUNCTION_NAME
   struct legacy_camera_device *lcdev = to_lcdev(device);
   LOGV("camera_start_preview: Enabling CAMERA_MSG_PREVIEW_FRAME\n");
   lcdev->hwif->enableMsgType(CAMERA_MSG_PREVIEW_FRAME);
   return lcdev->hwif->startPreview();
}

void camera_stop_preview(struct camera_device * device)
{
	LOG_FUNCTION_NAME
   struct legacy_camera_device *lcdev = to_lcdev(device);
   LOGV("camera_stop_preview:\n");
   lcdev->hwif->disableMsgType(CAMERA_MSG_PREVIEW_FRAME);
   lcdev->hwif->stopPreview();
   return;
}

int camera_preview_enabled(struct camera_device * device)
{
	LOG_FUNCTION_NAME
   struct legacy_camera_device *lcdev = to_lcdev(device);
   int ret = lcdev->hwif->previewEnabled();
   LOGV("%s: %d\n",__FUNCTION__,ret);
   return ret;
}

int camera_store_meta_data_in_buffers(struct camera_device * device, int enable)
{
	LOG_FUNCTION_NAME
	return NO_ERROR;
}

int camera_start_recording(struct camera_device * device)
{
	LOG_FUNCTION_NAME
	return NO_ERROR;
}

void camera_stop_recording(struct camera_device * device)
{
	LOG_FUNCTION_NAME
}

int camera_recording_enabled(struct camera_device * device)
{
	LOG_FUNCTION_NAME
	return NO_ERROR;
}

void camera_release_recording_frame(struct camera_device * device,
                                const void *opaque)
{
	LOG_FUNCTION_NAME
}

int camera_auto_focus(struct camera_device * device)
{
	return NO_ERROR;
}

int camera_cancel_auto_focus(struct camera_device * device)
{
	LOG_FUNCTION_NAME
	return NO_ERROR;
}

int camera_take_picture(struct camera_device * device)
{
	LOG_FUNCTION_NAME
	return NO_ERROR;
}

int camera_cancel_picture(struct camera_device * device)
{
	LOG_FUNCTION_NAME
	return NO_ERROR;
}

int camera_set_parameters(struct camera_device * device, const char *params)
{
	LOG_FUNCTION_NAME
	return NO_ERROR;
}

char* camera_get_parameters(struct camera_device * device)
{
   LOG_FUNCTION_NAME
   struct legacy_camera_device *lcdev = to_lcdev(device);
   char *rc = NULL;
   android::CameraParameters params(lcdev->hwif->getParameters());
   camera_fixup_params(params);
   rc = strdup((char *)params.flatten().string());
   LOGV("%s: returning rc:%p :%s\n",__FUNCTION__,
        rc, (rc != NULL) ? rc : "EMPTY STRING");
   return rc;
}

void camera_put_parameters(struct camera_device *device, char *params)
{
   LOG_FUNCTION_NAME
   LOGV("%s: params:%p %s",__FUNCTION__ , params, params);
   free(params);
}

int camera_send_command(struct camera_device * device, int32_t cmd,
                        int32_t arg0, int32_t arg1)
{
	LOG_FUNCTION_NAME
	return NO_ERROR;
}

void camera_release(struct camera_device * device)
{
	LOG_FUNCTION_NAME
}

int camera_dump(struct camera_device * device, int fd)
{
	LOG_FUNCTION_NAME
	return NO_ERROR;
}

int camera_device_close(hw_device_t* device)
{
	LOG_FUNCTION_NAME
	return NO_ERROR;
}

int camera_device_open(const hw_module_t* module, const char* name,
                   hw_device_t** device)
{
	LOG_FUNCTION_NAME
   	int ret;
   	struct legacy_camera_device *lcdev;
   	camera_device_t* camera_device;
   	camera_device_ops_t* camera_ops;

   	if (name == NULL)
      	return 0;

   	int cameraId = atoi(name);

   	LOGD("%s: name:%s device:%p cameraId:%d\n",__FUNCTION__,
        name, device, cameraId);

   	lcdev = (struct legacy_camera_device *)calloc(1, sizeof(*lcdev));
   	camera_ops = (camera_device_ops_t *)malloc(sizeof(*camera_ops));
   	memset(camera_ops, 0, sizeof(*camera_ops));

   	lcdev->device.common.tag               = HARDWARE_DEVICE_TAG;
   	lcdev->device.common.version           = 0;
   	lcdev->device.common.module            = (hw_module_t *)(module);
   	lcdev->device.common.close             = camera_device_close;
   	lcdev->device.ops                      = camera_ops;

   	camera_ops->set_preview_window         = camera_set_preview_window;
   	camera_ops->set_callbacks              = camera_set_callbacks;
   	camera_ops->enable_msg_type            = camera_enable_msg_type;
   	camera_ops->disable_msg_type           = camera_disable_msg_type;
   	camera_ops->msg_type_enabled           = camera_msg_type_enabled;
   	camera_ops->start_preview              = camera_start_preview;
   	camera_ops->stop_preview               = camera_stop_preview;
   	camera_ops->preview_enabled            = camera_preview_enabled;
   	camera_ops->store_meta_data_in_buffers = camera_store_meta_data_in_buffers;
   	camera_ops->start_recording            = camera_start_recording;
   	camera_ops->stop_recording             = camera_stop_recording;
   	camera_ops->recording_enabled          = camera_recording_enabled;
   	camera_ops->release_recording_frame    = camera_release_recording_frame;
   	camera_ops->auto_focus                 = camera_auto_focus;
   	camera_ops->cancel_auto_focus          = camera_cancel_auto_focus;
   	camera_ops->take_picture               = camera_take_picture;
   	camera_ops->cancel_picture             = camera_cancel_picture;
   	camera_ops->set_parameters             = camera_set_parameters;
   	camera_ops->get_parameters             = camera_get_parameters;
   	camera_ops->put_parameters             = camera_put_parameters;
   	camera_ops->send_command               = camera_send_command;
   	camera_ops->release                    = camera_release;
   	camera_ops->dump                       = camera_dump;

   	lcdev->id = cameraId;
   	lcdev->hwif = HAL_openCameraHardware(cameraId);
   	if (lcdev->hwif == NULL) {
       	ret = -EIO;
       	goto err_create_camera_hw;
   	}
   	*device = &lcdev->device.common;
   	return NO_ERROR;

err_create_camera_hw:
   	free(lcdev);
   	free(camera_ops);
   	return ret;
}
