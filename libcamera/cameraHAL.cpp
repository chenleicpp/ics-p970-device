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
#include <fcntl.h>
#include <linux/ioctl.h>
#include <hardware/gralloc.h>

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
	return NO_ERROR;
}

void camera_set_callbacks(struct camera_device * device,
                      camera_notify_callback notify_cb,
                      camera_data_callback data_cb,
                      camera_data_timestamp_callback data_cb_timestamp,
                      camera_request_memory get_memory, void *user)
{

}

void camera_enable_msg_type(struct camera_device * device, int32_t msg_type)
{

}

void camera_disable_msg_type(struct camera_device * device, int32_t msg_type)
{

}

int camera_msg_type_enabled(struct camera_device * device, int32_t msg_type)
{
	return NO_ERROR;
}

int camera_start_preview(struct camera_device * device)
{
	return NO_ERROR;
}

void camera_stop_preview(struct camera_device * device)
{

}

int camera_preview_enabled(struct camera_device * device)
{
	return NO_ERROR;
}

int camera_store_meta_data_in_buffers(struct camera_device * device, int enable)
{
	return NO_ERROR;
}

int camera_start_recording(struct camera_device * device)
{
	return NO_ERROR;
}

void camera_stop_recording(struct camera_device * device)
{
	
}

int camera_recording_enabled(struct camera_device * device)
{
	return NO_ERROR;
}

void camera_release_recording_frame(struct camera_device * device,
                                const void *opaque)
{

}

int camera_auto_focus(struct camera_device * device)
{
	return NO_ERROR;
}

int camera_cancel_auto_focus(struct camera_device * device)
{
	return NO_ERROR;
}

int camera_take_picture(struct camera_device * device)
{
	return NO_ERROR;
}

int camera_cancel_picture(struct camera_device * device)
{
	return NO_ERROR;
}

int camera_set_parameters(struct camera_device * device, const char *params)
{
	return NO_ERROR;
}

char* camera_get_parameters(struct camera_device * device)
{
	return NULL;
}

void camera_put_parameters(struct camera_device *device, char *params)
{

}

int camera_send_command(struct camera_device * device, int32_t cmd,
                        int32_t arg0, int32_t arg1)
{
	return NO_ERROR;
}

void camera_release(struct camera_device * device)
{

}

int camera_dump(struct camera_device * device, int fd)
{
	return NO_ERROR;
}

int camera_device_close(hw_device_t* device)
{
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