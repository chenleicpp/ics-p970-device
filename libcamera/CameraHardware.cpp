/*
**
** Copyright (C) 2009 0xlab.org - http://0xlab.org/
** Copyright 2008, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#define LOG_TAG "***CameraHardware***"
#include <utils/Log.h>

#include "CameraHardware.h"
#include "converter.h"
#include <fcntl.h>
#include <sys/mman.h>

#include <cutils/native_handle.h>
#include <hal_public.h>
#include <ui/GraphicBufferMapper.h>
#include <gui/ISurfaceTexture.h>

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
//chenlei : video0 is front cam and video5 is back cam
#define VIDEO_DEVICE_2      "/dev/video0"
#define VIDEO_DEVICE_0      "/dev/video5"
#define MEDIA_DEVICE        "/dev/media0"
#define PREVIEW_WIDTH       640
#define PREVIEW_HEIGHT      480
//chenlei : use kernel format
#define PIXEL_FORMAT       V4L2_PIX_FMT_UYVY
//Define Cameras
#define CAMERA_BF 0		//Back Camera
#define CAMERA_FF 1

static const int INITIAL_SKIP_FRAME = 3;

#define CAMHAL_GRALLOC_USAGE GRALLOC_USAGE_HW_TEXTURE | \
                             GRALLOC_USAGE_HW_RENDER | \
                             GRALLOC_USAGE_SW_READ_RARELY | \
                             GRALLOC_USAGE_SW_WRITE_NEVER

#ifndef KERNEL_VERSION
#define KERNEL_VERSION(a,b,c) (((a) << 16) + ((b) << 8) + (c))
#endif
#define MAX_STR_LEN 35
#define EXIF_FILE_SIZE 28800

#define BACK_CAMERA_AUTO_FOCUS_DISTANCES_STR "0.10,1.20,Infinity"
#define BACK_CAMERA_MACRO_FOCUS_DISTANCES_STR "0.10,0.20,Infinity"
#define BACK_CAMERA_INFINITY_FOCUS_DISTANCES_STR "0.10,1.20,Infinity"
#define FRONT_CAMERA_FOCUS_DISTANCES_STR "0.20,0.25,Infinity"
const char FOCUS_MODE_FACEDETECTION[] = "facedetect";

#include <cutils/properties.h>
#ifndef UNLIKELY
#define UNLIKELY(exp) (__builtin_expect( (exp) != 0, false ))
#endif
static int mDebugFps = 0;
static int mCameraID = 0;
int version = 0;
namespace android {

/* 29/12/10 : preview/picture size validation logic */
const char CameraHardware::supportedPictureSizes_ffc [] = "640x480";
const char CameraHardware::supportedPictureSizes_bfc [] = "640x480";
const char CameraHardware::supportedPreviewSizes_ffc [] = "640x480,320x240,176x144";
const char CameraHardware::supportedPreviewSizes_bfc [] = "1280x720,800x480,720x480,640x480,352x288";

CameraHardware::CameraHardware(int CameraID)
	:mParameters(),
	mHeap(0),
	mPreviewHeap(0),
	mRawHeap(0),
	mCamera(0),
	mPreviewFrameSize(0),
	mNotifyCb(0),
	mDataCb(0),
	mDataCbTimestamp(0),
	mCallbackCookie(0),
	mMsgEnabled(0),
	previewStopped(true), mRecordingEnabled(false), mSkipFrame(0) {
	/* create camera */
	mCamera = new V4L2Camera();
	version = get_kernel_version();
	mCameraID = CameraID;
	if (CameraID == CAMERA_FF) {
		mCamera->Open(VIDEO_DEVICE_2);
	} else {
		mCamera->Open(VIDEO_DEVICE_0);
	}

	initDefaultParameters(CameraID);
	mNativeWindow = NULL;
	for (int i = 0; i < NB_BUFFER; i++) {
		mRecordHeap[i] = NULL;
		mRecordBufferState[i] = 0;
	}

	mExitAutoFocusThread = false;
	mAutoFocusThread = new AutoFocusThread(this);
	mPictureThread = new PictureThread(this);

	/* whether prop "debug.camera.showfps" is enabled or not */
	char value[PROPERTY_VALUE_MAX];
	property_get("debug.camera.showfps", value, "0");
	mDebugFps = atoi(value);
	LOGD_IF(mDebugFps, "showfps enabled");
}

void CameraHardware::initDefaultParameters(int CameraID) {
	CameraParameters p;
	String8 parameterString;
	if (CameraID == CAMERA_FF) {
		p.setPreviewFormat(CameraParameters::
				   PIXEL_FORMAT_YUV420SP);
		p.setPictureSize(PREVIEW_WIDTH, PREVIEW_HEIGHT);
		p.setPictureFormat(CameraParameters::PIXEL_FORMAT_JPEG);
		p.set(CameraParameters::KEY_JPEG_QUALITY, 90);
		p.set("picture-size-values",
			  CameraHardware::supportedPictureSizes_ffc);

		p.set(CameraParameters::KEY_SUPPORTED_PICTURE_SIZES,
			  CameraHardware::supportedPictureSizes_ffc);
		p.set(CameraParameters::KEY_SUPPORTED_PICTURE_FORMATS,
			  CameraParameters::PIXEL_FORMAT_JPEG);
		p.set(CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES,
			  CameraHardware::supportedPreviewSizes_ffc);
		p.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FORMATS,
			  CameraParameters::PIXEL_FORMAT_YUV420SP);
		p.set(CameraParameters::KEY_VIDEO_FRAME_FORMAT,
			  CameraParameters::PIXEL_FORMAT_YUV422I);

		parameterString = CameraParameters::FOCUS_MODE_FIXED;
		p.set(CameraParameters::KEY_SUPPORTED_FOCUS_MODES,
			  parameterString.string());
		p.set(CameraParameters::KEY_FOCUS_MODE,
			  CameraParameters::FOCUS_MODE_FIXED);
		p.set(CameraParameters::KEY_FOCUS_DISTANCES,
			  FRONT_CAMERA_FOCUS_DISTANCES_STR);
		p.set(CameraParameters::
			  KEY_SUPPORTED_JPEG_THUMBNAIL_SIZES,
			  "160x120,0x0");
		p.set(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH,
			  "160");
		p.set(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT,
			  "120");
		p.set(CameraParameters::
			  KEY_SUPPORTED_PREVIEW_FRAME_RATES, "15");
		p.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FPS_RANGE,
			  "(7500,30000)");
		p.set(CameraParameters::KEY_PREVIEW_FPS_RANGE,
			  "30000,30000");

		p.set(CameraParameters::KEY_FOCAL_LENGTH, "3.67");

		p.setPreviewSize(PREVIEW_WIDTH, PREVIEW_HEIGHT);
		p.setPreviewFrameRate(30);

	} else {
		p.setPreviewFormat(CameraParameters::
				   PIXEL_FORMAT_YUV420SP);
		p.setPictureSize(PREVIEW_WIDTH, PREVIEW_HEIGHT);
		p.setPictureFormat(CameraParameters::PIXEL_FORMAT_JPEG);
		p.set(CameraParameters::KEY_JPEG_QUALITY, 90);
		p.set("picture-size-values",
			  CameraHardware::supportedPictureSizes_bfc);

    	parameterString = CameraParameters::FOCUS_MODE_AUTO;
        parameterString.append(",");
        parameterString.append(CameraParameters::FOCUS_MODE_MACRO);
		parameterString.append(",");
		parameterString.append(FOCUS_MODE_FACEDETECTION);
		p.set(CameraParameters::KEY_SUPPORTED_FOCUS_MODES,
              		parameterString.string());
        p.set(CameraParameters::KEY_FOCUS_MODE,
              			CameraParameters::FOCUS_MODE_AUTO);

		p.set(CameraParameters::KEY_SUPPORTED_PICTURE_SIZES,
			  CameraHardware::supportedPictureSizes_bfc);
		p.set(CameraParameters::KEY_SUPPORTED_PICTURE_FORMATS,
			  CameraParameters::PIXEL_FORMAT_JPEG);
		p.set(CameraParameters::KEY_SUPPORTED_PREVIEW_SIZES,
			  CameraHardware::supportedPreviewSizes_bfc);
		p.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FORMATS,
			  CameraParameters::PIXEL_FORMAT_YUV420SP);
			  
		p.set(CameraParameters::
			  KEY_SUPPORTED_JPEG_THUMBNAIL_SIZES,
			  "320x240,0x0");
		p.set(CameraParameters::KEY_JPEG_THUMBNAIL_WIDTH,
			  "320");
		p.set(CameraParameters::KEY_JPEG_THUMBNAIL_HEIGHT,
			  "240");
		p.set(CameraParameters::
			  KEY_SUPPORTED_PREVIEW_FRAME_RATES, "30");
		p.set(CameraParameters::KEY_SUPPORTED_PREVIEW_FPS_RANGE,
			  "(15000,30000)");
		p.set(CameraParameters::KEY_PREVIEW_FPS_RANGE,
			  "30000,30000");

		p.set(CameraParameters::KEY_FOCAL_LENGTH, "3.67");

		p.setPreviewSize(PREVIEW_WIDTH, PREVIEW_HEIGHT);
		p.setPreviewFrameRate(30);
		/**p970 dont need this
		// touch focus
        p.set(CameraParameters::KEY_MAX_NUM_FOCUS_AREAS, "1");
        p.set(CameraParameters::KEY_FOCUS_AREAS, "(0,0,0,0,0)");
        **/
		// zoom	
		p.set(CameraParameters::KEY_ZOOM, "0");
		p.set(CameraParameters::KEY_MAX_ZOOM, "12");
		p.set(CameraParameters::KEY_ZOOM_RATIOS,
			  "100,125,150,175,200,225,250,275,300,325,350,375,400");
		p.set(CameraParameters::KEY_ZOOM_SUPPORTED, "true");

		parameterString = CameraParameters::SCENE_MODE_AUTO;
		parameterString.append(",");
		parameterString.
			append(CameraParameters::SCENE_MODE_PORTRAIT);
		parameterString.append(",");
		parameterString.
			append(CameraParameters::SCENE_MODE_LANDSCAPE);
		parameterString.append(",");
		parameterString.
			append(CameraParameters::SCENE_MODE_NIGHT);
		parameterString.append(",");
		parameterString.
			append(CameraParameters::SCENE_MODE_BEACH);
		parameterString.append(",");
		parameterString.
			append(CameraParameters::SCENE_MODE_SNOW);
		parameterString.append(",");
		parameterString.
			append(CameraParameters::SCENE_MODE_SUNSET);
		parameterString.append(",");
		parameterString.
			append(CameraParameters::SCENE_MODE_FIREWORKS);
		parameterString.append(",");
		parameterString.
			append(CameraParameters::SCENE_MODE_SPORTS);
		parameterString.append(",");
		parameterString.
			append(CameraParameters::SCENE_MODE_PARTY);
		parameterString.append(",");
		parameterString.
			append(CameraParameters::SCENE_MODE_CANDLELIGHT);
		p.set(CameraParameters::KEY_SUPPORTED_SCENE_MODES,
			  parameterString.string());
	}
	p.set(CameraParameters::KEY_SCENE_MODE,
		  CameraParameters::SCENE_MODE_AUTO);
	parameterString = CameraParameters::EFFECT_NONE;
	parameterString.append(",");
	parameterString.append(CameraParameters::EFFECT_MONO);
	parameterString.append(",");
	parameterString.append(CameraParameters::EFFECT_NEGATIVE);
	parameterString.append(",");
	parameterString.append(CameraParameters::EFFECT_SEPIA);
	p.set(CameraParameters::KEY_SUPPORTED_EFFECTS,
		  parameterString.string());

	parameterString = CameraParameters::WHITE_BALANCE_AUTO;
	parameterString.append(",");
	parameterString.
		append(CameraParameters::WHITE_BALANCE_INCANDESCENT);
	parameterString.append(",");
	parameterString.
		append(CameraParameters::WHITE_BALANCE_FLUORESCENT);
	parameterString.append(",");
	parameterString.
		append(CameraParameters::WHITE_BALANCE_DAYLIGHT);
	parameterString.append(",");
	parameterString.
		append(CameraParameters::WHITE_BALANCE_CLOUDY_DAYLIGHT);
	p.set(CameraParameters::KEY_SUPPORTED_WHITE_BALANCE,
		  parameterString.string());

	//p970: fix this params from cm7 camera settings
	p.set(CameraParameters::KEY_EXPOSURE_COMPENSATION, "0");
	p.set(CameraParameters::KEY_MAX_EXPOSURE_COMPENSATION, "20");
	p.set(CameraParameters::KEY_MIN_EXPOSURE_COMPENSATION, "-20");
	p.set(CameraParameters::KEY_EXPOSURE_COMPENSATION_STEP, "0.1");
	p.set(CameraParameters::KEY_HORIZONTAL_VIEW_ANGLE, "47.11");
	p.set(CameraParameters::KEY_VERTICAL_VIEW_ANGLE, "36.21");

	p.set(p.KEY_ROTATION, "0");
	p.set(p.KEY_GPS_LATITUDE, "0");
	p.set(p.KEY_GPS_LONGITUDE, "0");
	p.set(p.KEY_GPS_ALTITUDE, "0");
	p.set(p.KEY_GPS_TIMESTAMP, "0");
	p.set(p.KEY_GPS_PROCESSING_METHOD, "GPS");
	
	/**p970:dont need galaxy sl parameters
	//Extra Parameters - GalaxySL
	mCamera->setISO(ISO_AUTO);
    mCamera->setMetering(METERING_CENTER);
    mCamera->setContrast(CONTRAST_DEFAULT);
    mCamera->setSharpness(SHARPNESS_DEFAULT);
    mCamera->setSaturation(SATURATION_DEFAULT);
    **/
	
	if (setParameters(p) != NO_ERROR) {
		LOGE("Failed to set default parameters?!");
	}
	return;
}

int CameraHardware::get_kernel_version() {
	char *verstring, *dummy;
	int fd;
	int major, minor, rev, ver = -1;
	if ((verstring = (char *)malloc(MAX_STR_LEN)) == NULL) {
		LOGE("Failed to allocate memory\n");
		return -1;
	}
	if ((dummy = (char *)malloc(MAX_STR_LEN)) == NULL) {
		LOGE("Failed to allocate memory\n");
		free(verstring);
		return -1;
	}

	if ((fd = open("/proc/version", O_RDONLY)) < 0) {
		LOGE("Failed to open file /proc/version\n");
		goto ret;
	}

	if (read(fd, verstring, MAX_STR_LEN) < 0) {
		LOGE("Failed to read kernel version string from /proc/version file\n");
		close(fd);
		goto ret;
	}
	close(fd);
	if (sscanf
		(verstring, "%s %s %d.%d.%d%s\n", dummy, dummy, &major,
		 &minor, &rev, dummy) != 6) {
		LOGE("Failed to read kernel version numbers\n");
		goto ret;
	}
	ver = KERNEL_VERSION(major, minor, rev);
ret:
	free(verstring);
	free(dummy);
	return ver;
}

CameraHardware::~CameraHardware() {
	mCamera->Uninit(0);
	mCamera->StopStreaming();
	mCamera->Close();
	delete mCamera;
	mCamera = 0;
}

sp < IMemoryHeap > CameraHardware::getPreviewHeap()const {
	LOGD("Preview Heap");
	return mPreviewHeap;
} 

sp < IMemoryHeap > CameraHardware::getRawHeap() const {
	LOGD("Raw Heap");
	return mRawHeap;
} 

void CameraHardware::setCallbacks(camera_notify_callback notify_cb,
					camera_data_callback data_cb,
					camera_data_timestamp_callback
					data_cb_timestamp,
					camera_request_memory get_memory,
					void *user) {
	Mutex::Autolock lock(mLock);
	mNotifyCb = notify_cb;
	mDataCb = data_cb;
	mDataCbTimestamp = data_cb_timestamp;
	mRequestMemory = get_memory;
	mCallbackCookie = user;
	return;
}

int CameraHardware::setPreviewWindow(preview_stream_ops_t * window) {
	int err;
	Mutex::Autolock lock(mLock);
	if (mNativeWindow)
		mNativeWindow = NULL;
	if (window == NULL) {
		LOGW("Window is Null");
		return 0;
	}

	int width, height;
	mParameters.getPreviewSize(&width, &height);
	mNativeWindow = window;
	mNativeWindow->set_usage(mNativeWindow,
				 GRALLOC_USAGE_SW_WRITE_OFTEN);
	mNativeWindow->set_buffers_geometry(mNativeWindow, width,
						height,
						HAL_PIXEL_FORMAT_YV12);
	err = mNativeWindow->set_buffer_count(mNativeWindow, 3);
	if (err != 0) {
		LOGE("native_window_set_buffer_count failed: %s (%d)",
			 strerror(-err), -err);

		if (ENODEV == err) {
			LOGE("Preview surface abandoned!");
			mNativeWindow = NULL;
		}
	}

	return 0;
}

void CameraHardware::enableMsgType(int32_t msgType) {
	LOGD("enableMsgType:%d", msgType);
	mMsgEnabled |= msgType;
}

void CameraHardware::disableMsgType(int32_t msgType) {
	LOGD("disableMsgType:%d", msgType);
	mMsgEnabled &= ~msgType;
}

bool CameraHardware::msgTypeEnabled(int32_t msgType) {
	LOGD("msgTypeEnabled:%d", msgType);
	return (mMsgEnabled & msgType);
}

bool CameraHardware::validateSize(size_t width, 
				size_t height,
				const supported_resolution * supRes,
				size_t count) {
	bool ret = false;
	status_t stat = NO_ERROR;
	unsigned int size;

	if (NULL == supRes) {
		LOGE("Invalid resolutions array passed");
		stat = -EINVAL;
	}

	if (NO_ERROR == stat) {
		for (unsigned int i = 0; i < count; i++) {
			// LOGD( "Validating %d, %d and %d, %d", supRes[i].width, width, supRes[i].height, height);
			if ((supRes[i].width == width)
				&& (supRes[i].height == height)) {
				ret = true;
				break;
			}
		}
	}
	return ret;
}

// ---------------------------------------------------------------------------
static void showFPS(const char *tag) {
	static int mFrameCount = 0;
	static int mLastFrameCount = 0;
	static nsecs_t mLastFpsTime = 0;
	static float mFps = 0;
	mFrameCount++;
	if (!(mFrameCount & 0x1F)) {
		nsecs_t now = systemTime();
		nsecs_t diff = now - mLastFpsTime;
		mFps =
			((mFrameCount -
			  mLastFrameCount) * float (s2ns(1)))/diff;
		mLastFpsTime = now;
		mLastFrameCount = mFrameCount;
		LOGD("[%s] %d Frames, %f FPS", tag, mFrameCount, mFps);
	}
}

int CameraHardware::previewThread()
{

    int width, height;
    int err;
    IMG_native_handle_t** hndl2hndl;
    IMG_native_handle_t* handle;
    int stride;
    int index;
    mParameters.getPreviewSize(&width, &height);
    int framesize= width * height * 1.5 ; //yuv420sp
    int mRecordFramesize= width * height * 2;

    mSkipFrameLock.lock();
    if (mSkipFrame > 0) {
        mSkipFrame--;
        void * mSkipFrameBuf=mCamera->GrabPreviewFrame();
        mSkipFrameLock.unlock();
        LOGV("%s: index %d skipping frame", __func__, index);
        return NO_ERROR;
    }
    mSkipFrameLock.unlock();

    if (!previewStopped) {

        mLock.lock();

        if (mNativeWindow != NULL) {

            if ((err = mNativeWindow->dequeue_buffer(mNativeWindow,(buffer_handle_t**) &hndl2hndl,&stride)) != 0) {
                LOGW("Surface::dequeueBuffer returned error %d", err);
                return -1;
            }

            mNativeWindow->lock_buffer(mNativeWindow, (buffer_handle_t*) hndl2hndl);
            GraphicBufferMapper &mapper = GraphicBufferMapper::get();
            Rect bounds(width, height);

            void *tempbuf;
            void *dst;

            if(0 == mapper.lock((buffer_handle_t)*hndl2hndl,GRALLOC_USAGE_SW_WRITE_OFTEN, bounds, &dst))
            {
				Mutex::Autolock lock(mRecordingLock);
				if(mRecordingEnabled)
				{
					tempbuf=mCamera->GrabRecordFrame(index);
					nsecs_t timeStamp = systemTime(SYSTEM_TIME_MONOTONIC);
					memcpy(mRecordHeap[index]->data,tempbuf,mRecordFramesize);
					mRecordBufferState[index]=1;

					if (mMsgEnabled & CAMERA_MSG_VIDEO_FRAME ) {
						mDataCbTimestamp(timeStamp, CAMERA_MSG_VIDEO_FRAME,mRecordHeap[index], 0, mCallbackCookie);
					}else{
						mCamera->ReleaseRecordFrame(index);
					}
				}else{
					// Get preview frame
					tempbuf=mCamera->GrabPreviewFrame();
				}
                yuv422_to_YV12((unsigned char *)tempbuf,(unsigned char *)dst, width, height);
                mapper.unlock((buffer_handle_t)*hndl2hndl);
                mNativeWindow->enqueue_buffer(mNativeWindow,(buffer_handle_t*) hndl2hndl);
            }

        }
        mLock.unlock();
    }
    return NO_ERROR;
}

status_t CameraHardware::startPreview() {

	int width, height;
	int mHeapSize = 0;
	int ret = 0;
	int fps = mParameters.getPreviewFrameRate();

	Mutex::Autolock lock(mPreviewLock);
	if (mPreviewThread != 0) {
		return INVALID_OPERATION;
	}

	mParameters.getPreviewSize(&mPreviewWidth, &mPreviewHeight);
	LOGD("startPreview width:%d,height:%d", mPreviewWidth,
		 mPreviewHeight);
	if (mPreviewWidth <= 0 || mPreviewHeight <= 0) {
		LOGE("Preview size is not valid,aborting..Device can not open!!!");
		return INVALID_OPERATION;
	}
	if (mCameraID == CAMERA_FF)
		fps = 15;
	ret =
		mCamera->Configure(mPreviewWidth, mPreviewHeight,
				   PIXEL_FORMAT, fps, 0);
	if (ret < 0) {
		LOGE("Fail to configure camera device");
		return INVALID_OPERATION;
	}
	//dhiru1602 : Set FFC Flip
	if (mCameraID == CAMERA_FF)
		mCamera->SetCameraFlip(false);
	/* clear previously buffers */
	if (mPreviewHeap != NULL) {
		LOGD("mPreviewHeap Cleaning!!!!");
		mPreviewHeap.clear();
	}

	if (mRawHeap != NULL) {
		LOGD("mRawHeap Cleaning!!!!");
		mRawHeap.clear();
	}

	if (mHeap != NULL) {
		LOGD("mHeap Cleaning!!!!");
		mHeap.clear();
	}

	mPreviewFrameSize = mPreviewWidth * mPreviewHeight * 2;
	mHeapSize = (mPreviewWidth * mPreviewHeight * 3) >> 1;

	/* mHead is yuv420 buffer, as default encoding is yuv420 */
	mHeap = new MemoryHeapBase(mHeapSize);
	mBuffer = new MemoryBase(mHeap, 0, mHeapSize);

	mPreviewHeap = new MemoryHeapBase(mPreviewFrameSize);
	mPreviewBuffer =
		new MemoryBase(mPreviewHeap, 0, mPreviewFrameSize);

	mRawHeap = new MemoryHeapBase(mPreviewFrameSize);
	mRawBuffer = new MemoryBase(mRawHeap, 0, mPreviewFrameSize);

	ret = mCamera->BufferMap(0);
	if (ret) {
		LOGE("Camera Init fail: %s", strerror(errno));
		return UNKNOWN_ERROR;
	}

	ret = mCamera->StartStreaming();
	if (ret) {
		LOGE("Camera StartStreaming fail: %s", strerror(errno));
		mCamera->Uninit(0);
		mCamera->Close();
		return UNKNOWN_ERROR;
	}

	setSkipFrame(INITIAL_SKIP_FRAME);

	/* start preview thread */
	previewStopped = false;
	mPreviewThread = new PreviewThread(this);

	return NO_ERROR;
}

void CameraHardware::stopPreview() {
	sp < PreviewThread > previewThread;
	{		/* scope for the lock */
		Mutex::Autolock lock(mPreviewLock);
		previewStopped = true;
		previewThread = mPreviewThread;
		LOGD("scope for the lock");
	}

	/* don't hold the lock while waiting for the thread to quit */
	if (previewThread != 0) {
		previewThread->requestExitAndWait();
		LOGD("previewThread->requestExitAndWait();");
	}

	if (mPreviewThread != 0) {
		mCamera->Uninit(0);
		mCamera->StopStreaming();
		LOGD(" mCamera->StopStreaming();");
	}

	Mutex::Autolock lock(mPreviewLock);
	mPreviewThread.clear();
	LOGD("return");
	return;
}

bool CameraHardware::previewEnabled() {
	Mutex::Autolock lock(mPreviewLock);
	return mPreviewThread != 0;
}

status_t CameraHardware::startRecording() {
	LOGE("startRecording");
	Mutex::Autolock lock(mRecordingLock);

	int width, height;
	mParameters.getPreviewSize(&width, &height);
	int mRecordingFrameSize = width * height * 2;

	for (int i = 0; i < NB_BUFFER; i++) {
		if (mRecordHeap[i] != NULL) {
			mRecordHeap[i]->release(mRecordHeap[i]);
			mRecordHeap[i] = 0;
		}
		mRecordBufferState[i] = 0;
		mRecordHeap[i] =
			mRequestMemory(-1, mRecordingFrameSize, 1, NULL);
	}

	//Skip the first recording frame since it is often garbled
	setSkipFrame(1);

	mRecordingEnabled = true;
	return NO_ERROR;

}

void CameraHardware::stopRecording() {
	LOGE("stopRecording");
	Mutex::Autolock lock(mRecordingLock);
	if (mRecordingEnabled) {
		for (int i = 0; i < NB_BUFFER; i++) {
			if (mRecordBufferState[i] != 0) {
				mCamera->ReleaseRecordFrame(i);
				mRecordBufferState[i] = 0;
			}
		}
		mRecordingEnabled = false;
	}
}

bool CameraHardware::recordingEnabled() {
	return mRecordingEnabled;
}

void CameraHardware::releaseRecordingFrame(const void *opaque) {
	int i;
	for (i = 0; i < NB_BUFFER; i++)
		if ((void *)opaque == mRecordHeap[i]->data)
			break;
	mCamera->ReleaseRecordFrame(i);
	mRecordBufferState[i] = 0;
}

// ---------------------------------------------------------------------------

int CameraHardware::beginAutoFocusThread(void *cookie) {
	CameraHardware *c = (CameraHardware *) cookie;
	LOGD("beginAutoFocusThread");
	return c->autoFocusThread();
}

int CameraHardware::autoFocusThread() {
	int count = 0;
	int af_status = 0;

	LOGD("%s : starting", __func__);

	//dhiru1602 :  Release focus lock when autofocus is called again.
	mCamera->setAEAWBLockUnlock(0, 0);

	/* block until we're told to start. we don't want to use
	 * a restartable thread and requestExitAndWait() in cancelAutoFocus()
	 * because it would cause deadlock between our callbacks and the
	 * caller of cancelAutoFocus() which both want to grab the same lock
	 * in CameraServices layer.
	 */

	mFocusLock.lock();
	/* check early exit request */
	if (mExitAutoFocusThread) {
		mFocusLock.unlock();
		LOGD("%s : exiting on request0", __func__);
		return NO_ERROR;
	}
	mFocusCondition.wait(mFocusLock);
	/* check early exit request */
	if (mExitAutoFocusThread) {
		mFocusLock.unlock();
		LOGD("%s : exiting on request1", __func__);
		return NO_ERROR;
	}
	mFocusLock.unlock();
	LOGD("%s : calling setAutoFocus", __func__);
	if (mCamera->setAutofocus() < 0) {
		LOGE("ERR(%s):Fail on mSecCamera->setAutofocus()",
			 __func__);
		return UNKNOWN_ERROR;
	}

	af_status = mCamera->getAutoFocusResult();

	if (af_status == 0x01) {
		LOGD("%s : AF Success!!", __func__);
		if (mMsgEnabled & CAMERA_MSG_FOCUS)
			mNotifyCb(CAMERA_MSG_FOCUS, true, 0,
				  mCallbackCookie);
	} else if (af_status == 0x02) {
		LOGD("%s : AF Cancelled !!", __func__);
		if (mMsgEnabled & CAMERA_MSG_FOCUS) {
			/* CAMERA_MSG_FOCUS only takes a bool. true for
			 * finished and false for failure. cancel is still
			 * considered a true result.
			 */
			mNotifyCb(CAMERA_MSG_FOCUS, true, 0,
				  mCallbackCookie);
		}
	} else {
		LOGD("%s : AF Fail !!", __func__);
		LOGD("%s : mMsgEnabled = 0x%x", __func__, mMsgEnabled);
		if (mMsgEnabled & CAMERA_MSG_FOCUS)
			mNotifyCb(CAMERA_MSG_FOCUS, false, 0,
				  mCallbackCookie);
	}

	LOGD("%s : exiting with no error", __func__);
	return NO_ERROR;
}

status_t CameraHardware::autoFocus() {

	mFocusCondition.signal();
	return NO_ERROR;
}

status_t CameraHardware::cancelAutoFocus() {

	if (mPreviewThread == NULL)
		return NO_ERROR;

	if (mCamera->cancelAutofocus() < 0) {
		LOGE("ERR(%s):Fail on mCamera->cancelAutofocus()",
			 __func__);
		return UNKNOWN_ERROR;
	}
	return NO_ERROR;
}

int CameraHardware::beginPictureThread(void *cookie) {
	CameraHardware *c = (CameraHardware *) cookie;
	LOGE("begin Picture Thread");
	return c->pictureThread();
}

int CameraHardware::pictureThread() {
	unsigned char *frame;
	int bufferSize;
	int w, h;
	int ret;
	struct v4l2_buffer buffer;
	struct v4l2_format format;
	struct v4l2_buffer cfilledbuffer;
	struct v4l2_requestbuffers creqbuf;
	struct v4l2_capability cap;
	int i;
	char devnode[12];
	camera_memory_t *picture;

	LOGD("Picture Thread:%d", mMsgEnabled);
	if (mMsgEnabled & CAMERA_MSG_SHUTTER)
		mNotifyCb(CAMERA_MSG_SHUTTER, 0, 0, mCallbackCookie);

	mParameters.getPictureSize(&w, &h);
	LOGD("Picture Size: Width = %d \t Height = %d", w, h);

	int width, height;
	mParameters.getPictureSize(&width, &height);
	int fps = mParameters.getPreviewFrameRate();
	int pixelformat = PIXEL_FORMAT;

	ret = mCamera->Configure(width, height, pixelformat, fps, 1);
	if (ret < 0) {
		LOGE("Fail to configure camera device");
		return INVALID_OPERATION;
	}
	//dhiru1602 : Set FFC Flip
	if (mCameraID == CAMERA_FF)
		mCamera->SetCameraFlip(true);

	ret = mCamera->BufferMap(1);
	if (ret) {
		LOGE("Camera BufferMap fail: %s", strerror(errno));
		return UNKNOWN_ERROR;
	}

	ret = mCamera->StartStreaming();
	if (ret) {
		LOGE("Camera StartStreaming fail: %s", strerror(errno));
		mCamera->Uninit(1);
		mCamera->Close();
		return UNKNOWN_ERROR;
	}

	camera_memory_t *tempbuf = NULL;

	if (mMsgEnabled & CAMERA_MSG_RAW_IMAGE) {
		mCamera->GrabRawFrame(tempbuf, w, h);
		mDataCb(CAMERA_MSG_RAW_IMAGE, tempbuf, 0, NULL,
			mCallbackCookie);
	} else if (mMsgEnabled & CAMERA_MSG_RAW_IMAGE_NOTIFY) {
		mNotifyCb(CAMERA_MSG_RAW_IMAGE_NOTIFY, 0, 0,
			  mCallbackCookie);
	}
	//TODO xxx : Optimize the memory capture call. Too many memcpy
	if (mMsgEnabled & CAMERA_MSG_COMPRESSED_IMAGE) {
		LOGD("mJpegPictureCallback");
		unsigned long JpegImageSize;
//			if (mCameraID == CAMERA_FF){  //by qiwu.huang
			picture =
				mCamera->GrabJpegFrame(mRequestMemory,
						   JpegImageSize, true);
//			}else{
//				picture =
//				    mCamera->GrabJpegFrame(mRequestMemory,
//							   JpegImageSize,
//							   false);
//						   }
		unsigned char *pExifBuf = new unsigned char[65536];
		int JpegExifSize;

		camera_memory_t *ExifHeap =
			mRequestMemory(-1, EXIF_FILE_SIZE, 1, 0);

		//TODO : dhiru1602- Include EXIF Thumbnail for JPEG Images      
		CreateExif(NULL, NULL, (unsigned char *)ExifHeap->data,
			   JpegExifSize, 1);

		LOGD("JpegExifSize=%d, JpegImageSize=%d", JpegExifSize,
			 (int)JpegImageSize);

		camera_memory_t *mem =
			mRequestMemory(-1,
				   (int)JpegImageSize + JpegExifSize, 1,
				   0);
		uint8_t *ptr = (uint8_t *) mem->data;
		memcpy(ptr, picture->data, 2);
		ptr += 2;
		memcpy(ptr, ExifHeap->data, JpegExifSize);
		ptr += JpegExifSize;
		memcpy(ptr, (uint8_t *) picture->data + 2,
			   (int)JpegImageSize - 2);

		mDataCb(CAMERA_MSG_COMPRESSED_IMAGE, mem, 0, NULL,
			mCallbackCookie);
	}

	/* Close operation */
	mCamera->Uninit(1);
	mCamera->StopStreaming();

	LOGD("End pictureThread()");
	return NO_ERROR;
}

status_t CameraHardware::takePicture() {
	LOGD("pictureThread()");
	stopPreview();
	if (mPictureThread->
		run("CameraPictureThread", PRIORITY_DEFAULT) != NO_ERROR) {
		LOGE("%s : couldn't run picture thread", __func__);
		return INVALID_OPERATION;
	}
	return NO_ERROR;
}

status_t CameraHardware::cancelPicture() {
	if (mPictureThread.get()) {
		LOGD("%s: waiting for picture thread to exit",
			 __func__);
		mPictureThread->requestExitAndWait();
		LOGD("%s: picture thread has exited", __func__);
	}
	return NO_ERROR;
}

status_t CameraHardware::dump(int fd, const Vector < String16 > &args)const {
	return NO_ERROR;
} status_t CameraHardware::
	setParameters(const CameraParameters & params) {
	int width = 0;
	int height = 0;
	int framerate = 0;
	int ret;
	params.getPreviewSize(&width, &height);

	LOGD("Set Parameter...!! ");

	LOGD("PreviewFormat %s", params.getPreviewFormat());
	if (params.getPreviewFormat() != NULL) {
		if (strcmp
			(params.getPreviewFormat(),
			 (const char *)CameraParameters::
			 PIXEL_FORMAT_YUV420SP) != 0) {
			LOGE("Only YUV420SP preview is supported");
			return -EINVAL;
		}
	}

	LOGD("PictureFormat %s", params.getPictureFormat());
	if (params.getPictureFormat() != NULL) {
		if (strcmp
			(params.getPictureFormat(),
			 (const char *)CameraParameters::
			 PIXEL_FORMAT_JPEG) != 0) {
			LOGE("Only jpeg still pictures are supported");
			return -EINVAL;
		}
	}

	framerate = params.getPreviewFrameRate();
	LOGD("FRAMERATE %d", framerate);

	mParameters = params;

	mParameters.getPictureSize(&width, &height);
	LOGD("Picture Size by CamHAL %d x %d", width, height);

	mParameters.getPreviewSize(&width, &height);
	LOGD("Preview Resolution by CamHAL %d x %d", width, height);
	mParameters.setPreviewSize(width, height);

	// whitebalance
	const char *new_white_str =
		params.get(CameraParameters::KEY_WHITE_BALANCE);
	LOGD("%s : new_white_str %s", __func__, new_white_str);
	if (new_white_str != NULL) {
		int new_white = -1;

		if (!strcmp
			(new_white_str,
			 (const char *)CameraParameters::
			 WHITE_BALANCE_AUTO))
			new_white = WHITE_BALANCE_AUTO;
		else if (!strcmp(new_white_str,
				 (const char *)CameraParameters::
				 WHITE_BALANCE_DAYLIGHT))
			new_white = WHITE_BALANCE_SUNNY;
		else if (!strcmp(new_white_str,
				 (const char *)CameraParameters::
				 WHITE_BALANCE_CLOUDY_DAYLIGHT))
			new_white = WHITE_BALANCE_CLOUDY;
		else if (!strcmp(new_white_str,
				 (const char *)CameraParameters::
				 WHITE_BALANCE_FLUORESCENT))
			new_white = WHITE_BALANCE_FLUORESCENT;
		else if (!strcmp(new_white_str,
				 (const char *)CameraParameters::
				 WHITE_BALANCE_INCANDESCENT))
			new_white = WHITE_BALANCE_TUNGSTEN;
		else {
			LOGE("ERR(%s):Invalid white balance(%s)", __func__, new_white_str);	//twilight, shade, warm_flourescent
		}

		if (0 <= new_white) {
			if (mCamera->setWhiteBalance(new_white) < 0) {
				LOGE("ERR(%s):Fail on mCamera->setWhiteBalance(white(%d))", __func__, new_white);
			} else {
				mParameters.
					set(CameraParameters::
					KEY_WHITE_BALANCE,
					new_white_str);
			}
		}
	}
	// brightness
	int new_exposure_compensation =
		params.getInt(CameraParameters::KEY_EXPOSURE_COMPENSATION);
	int max_exposure_compensation =
		params.
		getInt(CameraParameters::KEY_MAX_EXPOSURE_COMPENSATION);
	int min_exposure_compensation =
		params.
		getInt(CameraParameters::KEY_MIN_EXPOSURE_COMPENSATION);
	LOGD("%s : new_exposure_compensation %d", __func__,
		 new_exposure_compensation);
	if ((min_exposure_compensation <= new_exposure_compensation)
		&& (max_exposure_compensation >=
		new_exposure_compensation)) {
		if (mCamera->setBrightness(new_exposure_compensation) <
			0) {
			LOGE("ERR(%s):Fail on mCamera->setBrightness(brightness(%d))", __func__, new_exposure_compensation);
		} else {
			mParameters.
				set(CameraParameters::
				KEY_EXPOSURE_COMPENSATION,
				new_exposure_compensation);
		}
	}
	// scene mode
	const char *new_scene_mode_str =
		params.get(CameraParameters::KEY_SCENE_MODE);
	const char *current_scene_mode_str =
		mParameters.get(CameraParameters::KEY_SCENE_MODE);

	// fps range
	int new_min_fps = 0;
	int new_max_fps = 0;
	int current_min_fps, current_max_fps;
	params.getPreviewFpsRange(&new_min_fps, &new_max_fps);
	mParameters.getPreviewFpsRange(&current_min_fps,
					   &current_max_fps);
	/* our fps range is determined by the sensor, reject any request
	 * that isn't exactly what we're already at.
	 * but the check is performed when requesting only changing fps range
	 */
	if ((new_min_fps > new_max_fps) || (new_min_fps < 0)
		|| (new_max_fps < 0)) {
		LOGE("WARN(%s): request for preview frame is not initial. \n", __func__);
		return UNKNOWN_ERROR;
	} else {
		mParameters.setPreviewFrameRate(new_max_fps / 1000);
	}

	if (new_scene_mode_str && current_scene_mode_str) {
		if (!strcmp(new_scene_mode_str, current_scene_mode_str)) {
			if ((new_min_fps != current_min_fps)
				|| (new_max_fps != current_max_fps)) {
				LOGW("%s : requested new_min_fps = %d, new_max_fps = %d not allowed", __func__, new_min_fps, new_max_fps);
				LOGE("%s : current_min_fps = %d, current_max_fps = %d", __func__, current_min_fps, current_max_fps);
				ret = UNKNOWN_ERROR;
			}
		}
	} else {
		/* Check basic validation if scene mode is different */
		if ((new_min_fps > new_max_fps) ||
			(new_min_fps < 0) || (new_max_fps < 0))
			ret = UNKNOWN_ERROR;
	}

	const char *new_focus_mode_str =
		params.get(CameraParameters::KEY_FOCUS_MODE);

	if (mCameraID == CAMERA_BF) {
		int new_scene_mode = -1;

		// fps range is (15000,30000) by default.
		mParameters.
			set(CameraParameters::
			KEY_SUPPORTED_PREVIEW_FPS_RANGE,
			"(15000,30000)");
		mParameters.set(CameraParameters::KEY_PREVIEW_FPS_RANGE,
				"15000,30000");

		if (!strcmp
			(new_scene_mode_str,
			 (const char *)CameraParameters::SCENE_MODE_AUTO)) {
			new_scene_mode = SCENE_MODE_NONE;
		} else {
			// defaults for non-auto scene modes
			if (mCameraID == CAMERA_BF) {
				new_focus_mode_str =
					CameraParameters::FOCUS_MODE_AUTO;
			}

			if (!strcmp(new_scene_mode_str,
					(const char *)CameraParameters::
					SCENE_MODE_PORTRAIT)) {
				new_scene_mode = SCENE_MODE_PORTRAIT;
			} else if (!strcmp(new_scene_mode_str,
					   (const char *)
					   CameraParameters::
					   SCENE_MODE_LANDSCAPE)) {
				new_scene_mode = SCENE_MODE_LANDSCAPE;
			} else if (!strcmp(new_scene_mode_str,
					   (const char *)
					   CameraParameters::
					   SCENE_MODE_SPORTS)) {
				new_scene_mode = SCENE_MODE_SPORTS;
			} else if (!strcmp(new_scene_mode_str,
					   (const char *)
					   CameraParameters::
					   SCENE_MODE_PARTY)) {
				new_scene_mode =
					SCENE_MODE_PARTY_INDOOR;
			} else
				if ((!strcmp
				 (new_scene_mode_str,
				  (const char *)CameraParameters::
				  SCENE_MODE_BEACH))
				||
				(!strcmp
				 (new_scene_mode_str,
				  (const char *)CameraParameters::
				  SCENE_MODE_SNOW))) {
				new_scene_mode = SCENE_MODE_BEACH_SNOW;
			} else if (!strcmp(new_scene_mode_str,
					   (const char *)
					   CameraParameters::
					   SCENE_MODE_SUNSET)) {
				new_scene_mode = SCENE_MODE_SUNSET;
			} else if (!strcmp(new_scene_mode_str,
					   (const char *)
					   CameraParameters::
					   SCENE_MODE_NIGHT)) {
				new_scene_mode = SCENE_MODE_NIGHTSHOT;
				mParameters.
					set(CameraParameters::
					KEY_SUPPORTED_PREVIEW_FPS_RANGE,
					"(4000,30000)");
				mParameters.
					set(CameraParameters::
					KEY_PREVIEW_FPS_RANGE,
					"4000,30000");
			} else
				if (!strcmp
				(new_scene_mode_str,
				 (const char *)CameraParameters::
				 SCENE_MODE_FIREWORKS)) {
				new_scene_mode = SCENE_MODE_FIREWORKS;
			} else if (!strcmp(new_scene_mode_str,
					   (const char *)
					   CameraParameters::
					   SCENE_MODE_CANDLELIGHT)) {
				new_scene_mode =
					SCENE_MODE_CANDLE_LIGHT;
			} else {
				LOGE("%s::unmatched scene_mode(%s)", __func__, new_scene_mode_str);	//action, night-portrait, theatre, steadyphoto
				ret = UNKNOWN_ERROR;
			}
		}

		// focus mode
		if (new_focus_mode_str != NULL) {
			int new_focus_mode = -1;

			if (!strcmp(new_focus_mode_str,
					(const char *)CameraParameters::
					FOCUS_MODE_AUTO)) {
				new_focus_mode = FOCUS_MODE_AUTO;
				mParameters.
					set(CameraParameters::
					KEY_FOCUS_DISTANCES,
					BACK_CAMERA_AUTO_FOCUS_DISTANCES_STR);
			} else
				if (!strcmp
				(new_focus_mode_str,
				 (const char *)CameraParameters::
				 FOCUS_MODE_MACRO)) {
				new_focus_mode = FOCUS_MODE_MACRO;
				mParameters.
					set(CameraParameters::
					KEY_FOCUS_DISTANCES,
					BACK_CAMERA_MACRO_FOCUS_DISTANCES_STR);
			} else
				if (!strcmp
				(new_focus_mode_str,
				 FOCUS_MODE_FACEDETECTION)) {
				new_focus_mode = FOCUS_MODE_FACEDETECT;
				mParameters.
					set(CameraParameters::
					KEY_FOCUS_DISTANCES,
					BACK_CAMERA_AUTO_FOCUS_DISTANCES_STR);
			} else {
				LOGE("%s::unmatched focus_mode(%s)",
					 __func__, new_focus_mode_str);
				ret = UNKNOWN_ERROR;
			}

			if (0 <= new_focus_mode) {
				if (mCamera->
					setFocusMode(new_focus_mode) < 0) {
					LOGE("%s::mCamera->setFocusMode(%d) fail", __func__, new_focus_mode);
					ret = UNKNOWN_ERROR;
				} else {
					mParameters.
						set(CameraParameters::
						KEY_FOCUS_MODE,
						new_focus_mode_str);
				}
			}
		}
		//  scene..
		if (0 <= new_scene_mode) {
			if (mCamera->setSceneMode(new_scene_mode) < 0) {
				LOGE("%s::mCamera->setSceneMode(%d) fail", __func__, new_scene_mode);
				ret = UNKNOWN_ERROR;
			} else {
				mParameters.
					set(CameraParameters::
					KEY_SCENE_MODE,
					new_scene_mode_str);
			}
		}
		//Camera Zoom Control
		int new_zoom =
			params.getInt(CameraParameters::KEY_ZOOM);
		int max_zoom =
			params.getInt(CameraParameters::KEY_MAX_ZOOM);
		LOGD("%s : new_zoom %d", __func__, new_zoom);
		if (0 <= new_zoom && new_zoom <= max_zoom) {
			LOGD("%s : set zoom:%d\n", __func__, new_zoom);
			if (mCamera->setZoom(new_zoom) < 0) {
				LOGE("ERR(%s):Fail on Camera->setZoom(%d)", __func__, new_zoom);
			} else {
				mParameters.
					set(CameraParameters::KEY_ZOOM,
					new_zoom);
			}
		}
	}
	// rotation
	int new_rotation =
		params.getInt(CameraParameters::KEY_ROTATION);
	if (0 <= new_rotation) {
		LOGD("%s : set orientation:%d\n", __func__,
			 new_rotation);
		if (mCamera->setExifOrientationInfo(new_rotation) < 0) {
			LOGE("ERR(%s):Fail on mCamera->setExifOrientationInfo(%d)", __func__, new_rotation);
			ret = UNKNOWN_ERROR;
		} else {
			mParameters.set(CameraParameters::KEY_ROTATION,
					new_rotation);
		}
	}

	return NO_ERROR;
}

CameraParameters CameraHardware::getParameters() const {
	CameraParameters params;

	{
		params = mParameters;
	} return params;
}

status_t CameraHardware::sendCommand(int32_t cmd, int32_t arg1,
					 int32_t arg2) {
	return BAD_VALUE;
}

void CameraHardware::release() {
	if (mPreviewThread != NULL) {
		mLock.lock();
		mPreviewThread->requestExit();
		mLock.unlock();
		mPreviewThread->requestExitAndWait();
		mPreviewThread.clear();
	}
	if (mAutoFocusThread != NULL) {
		mFocusLock.lock();
		mAutoFocusThread->requestExit();
		mExitAutoFocusThread = true;
		mFocusCondition.signal();
		mFocusLock.unlock();
		mAutoFocusThread->requestExitAndWait();
		mAutoFocusThread.clear();
	}
	if (mPictureThread != NULL) {
		mPictureThread->requestExitAndWait();
		mPictureThread.clear();
	}
	for (int i = 0; i < NB_BUFFER; i++) {
		if (mRecordHeap[i] != NULL) {
			mRecordHeap[i]->release(mRecordHeap[i]);
			mRecordHeap[i] = NULL;
		}
	}
	mCamera->Uninit(0);
	mCamera->Close();
}

void CameraHardware::CreateExif(unsigned char *pInThumbnailData,
				int Inthumbsize,
				unsigned char *pOutExifBuf,
				int &OutExifSize, int flag) {
	int w = 0, h = 0;

	int orientationValue = mCamera->getOrientation();

	ExifCreator *mExifCreator = new ExifCreator();
	unsigned int ExifSize = 0;
	ExifInfoStructure ExifInfo;
	char ver_date[5] = { NULL, };
	unsigned short tempISO = 0;
	struct v4l2_exif exifobj;

	// To read values from driver
	if (mCameraID == CAMERA_BF) {
		mCamera->getExifInfoFromDriver(&exifobj);
	}

	memset(&ExifInfo, NULL, sizeof(ExifInfoStructure));

	strcpy((char *)&ExifInfo.maker, "LG");
	strcpy((char *)&ExifInfo.model, "p970");

	mParameters.getPreviewSize(&w, &h);

	mParameters.getPictureSize((int *)&ExifInfo.imageWidth,
				   (int *)&ExifInfo.imageHeight);
	mParameters.getPictureSize((int *)&ExifInfo.pixelXDimension,
				   (int *)&ExifInfo.pixelYDimension);

	struct tm *t = NULL;
	time_t nTime;
	time(&nTime);
	t = localtime(&nTime);

	if (t != NULL) {
		sprintf((char *)&ExifInfo.dateTimeOriginal,
			"%4d:%02d:%02d %02d:%02d:%02d",
			t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
			t->tm_hour, t->tm_min, t->tm_sec);
		sprintf((char *)&ExifInfo.dateTimeDigitized,
			"%4d:%02d:%02d %02d:%02d:%02d",
			t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
			t->tm_hour, t->tm_min, t->tm_sec);
		sprintf((char *)&ExifInfo.dateTime,
			"%4d:%02d:%02d %02d:%02d:%02d",
			t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
			t->tm_hour, t->tm_min, t->tm_sec);
	}

	if (mCameraID == CAMERA_BF) {
		int cam_ver = mCamera->GetCamera_version();

		ExifInfo.Camversion[0] = (cam_ver & 0xFF);
		ExifInfo.Camversion[1] = ((cam_ver >> 8) & 0xFF);
		ExifInfo.Camversion[2] = ((cam_ver >> 16) & 0xFF);
		ExifInfo.Camversion[3] = ((cam_ver >> 24) & 0xFF);

		sprintf((char *)&ExifInfo.software,
			"fw %02d.%02d prm %02d.%02d",
			ExifInfo.Camversion[2], ExifInfo.Camversion[3],
			ExifInfo.Camversion[0], ExifInfo.Camversion[1]);
		if (mThumbnailWidth > 0 && mThumbnailHeight > 0) {
			ExifInfo.hasThumbnail = true;
			ExifInfo.thumbStream = pInThumbnailData;
			ExifInfo.thumbSize = Inthumbsize;
			ExifInfo.thumbImageWidth = mThumbnailWidth;
			ExifInfo.thumbImageHeight = mThumbnailHeight;
		} else {
			ExifInfo.hasThumbnail = false;
		}

		ExifInfo.exposureProgram = 3;
		ExifInfo.exposureMode = 0;
		ExifInfo.contrast =
			convertToExifLMH(mCamera->getContrast(), 2);
		ExifInfo.fNumber.numerator = 26;
		ExifInfo.fNumber.denominator = 10;
		ExifInfo.aperture.numerator = 26;
		ExifInfo.aperture.denominator = 10;
		ExifInfo.maxAperture.numerator = 26;
		ExifInfo.maxAperture.denominator = 10;
		ExifInfo.focalLength.numerator = 3430;
		ExifInfo.focalLength.denominator = 1000;
		//[ 2010 05 01 exif
		ExifInfo.shutterSpeed.numerator = exifobj.TV_Value;
		ExifInfo.shutterSpeed.denominator = 100;
		ExifInfo.exposureTime.numerator = 1;
		ExifInfo.exposureTime.denominator =
			(unsigned int)pow(2.0,
					  ((double)exifobj.TV_Value /
					   100.0));
		//]
		ExifInfo.brightness.numerator = 5;
		ExifInfo.brightness.denominator = 9;
		ExifInfo.iso = 1;
		ExifInfo.flash = 0;	// default value

		// Flash
		// bit 0    -whether the flash fired
		// bit 1,2 -status of returned light
		// bit 3,4 - indicating the camera's flash mode
		// bit 5    -presence of a flash function
		// bit 6    - red-eye mode

		// refer to flash_mode[] at CameraHal.cpp
		// off = 1
		// on = 2
		// auto = 3

		// Todo : Need to implement how HAL can recognize existance of flash
		//              if( ! isFlashExist )    // pseudo code
		//                      ExifInfo.flash = 32;            // bit 5 - No flash function.
		//              else
		{
			LOGD("createExif - flashmode = %d flash result = %d", mPreviousFlashMode, ExifInfo.flash);

			// bit 0
			ExifInfo.flash = ExifInfo.flash | exifobj.flash;
			// bit 3,4
			if (mPreviousFlashMode == 3)	// Flashmode auto
				ExifInfo.flash = ExifInfo.flash | 24;
			// bit 6
			// Todo : Need to implement about red-eye                       
			//                      if(mPreviousFlashMode == ??)    // Flashmode red-eye
			//                              ExifInfo.flash = ExifInfo.flash | 64;                                           
		}

		switch (orientationValue) {
		case 0:
			ExifInfo.orientation = 1;
			break;
		case 90:
			ExifInfo.orientation = 6;
			break;
		case 180:
			ExifInfo.orientation = 3;
			break;
		case 270:
			ExifInfo.orientation = 8;
			break;
		default:
			ExifInfo.orientation = 1;
			break;
		}
		//[ 2010 05 01 exif
		double calIsoValue = 0;
		calIsoValue =
			pow(2.0,
			((double)exifobj.SV_Value / 100.0)) * 3.125;
		//]
		if (calIsoValue < 8.909) {
			tempISO = 0;
		} else if (calIsoValue >= 8.909 && calIsoValue < 11.22) {
			tempISO = 10;
		} else if (calIsoValue >= 11.22 && calIsoValue < 14.14) {
			tempISO = 12;
		} else if (calIsoValue >= 14.14 && calIsoValue < 17.82) {
			tempISO = 16;
		} else if (calIsoValue >= 17.82 && calIsoValue < 22.45) {
			tempISO = 20;
		} else if (calIsoValue >= 22.45 && calIsoValue < 28.28) {
			tempISO = 25;
		} else if (calIsoValue >= 28.28 && calIsoValue < 35.64) {
			tempISO = 32;
		} else if (calIsoValue >= 35.64 && calIsoValue < 44.90) {
			tempISO = 40;
		} else if (calIsoValue >= 44.90 && calIsoValue < 56.57) {
			tempISO = 50;
		} else if (calIsoValue >= 56.57 && calIsoValue < 71.27) {
			tempISO = 64;
		} else if (calIsoValue >= 71.27 && calIsoValue < 89.09) {
			tempISO = 80;
		} else if (calIsoValue >= 89.09 && calIsoValue < 112.2) {
			tempISO = 100;
		} else if (calIsoValue >= 112.2 && calIsoValue < 141.4) {
			tempISO = 125;
		} else if (calIsoValue >= 141.4 && calIsoValue < 178.2) {
			tempISO = 160;
		} else if (calIsoValue >= 178.2 && calIsoValue < 224.5) {
			tempISO = 200;
		} else if (calIsoValue >= 224.5 && calIsoValue < 282.8) {
			tempISO = 250;
		} else if (calIsoValue >= 282.8 && calIsoValue < 356.4) {
			tempISO = 320;
		} else if (calIsoValue >= 356.4 && calIsoValue < 449.0) {
			tempISO = 400;
		} else if (calIsoValue >= 449.0 && calIsoValue < 565.7) {
			tempISO = 500;
		} else if (calIsoValue >= 565.7 && calIsoValue < 712.7) {
			tempISO = 640;
		} else if (calIsoValue >= 712.7 && calIsoValue < 890.9) {
			tempISO = 800;
		} else if (calIsoValue >= 890.9 && calIsoValue < 1122) {
			tempISO = 1000;
		} else if (calIsoValue >= 1122 && calIsoValue < 1414) {
			tempISO = 1250;
		} else if (calIsoValue >= 1414 && calIsoValue < 1782) {
			tempISO = 160;
		} else if (calIsoValue >= 1782 && calIsoValue < 2245) {
			tempISO = 2000;
		} else if (calIsoValue >= 2245 && calIsoValue < 2828) {
			tempISO = 2500;
		} else if (calIsoValue >= 2828 && calIsoValue < 3564) {
			tempISO = 3200;
		} else if (calIsoValue >= 3564 && calIsoValue < 4490) {
			tempISO = 4000;
		} else if (calIsoValue >= 4490 && calIsoValue < 5657) {
			tempISO = 5000;
		} else if (calIsoValue >= 5657 && calIsoValue < 7127) {
			tempISO = 6400;
		} else {
			tempISO = 8000;
		}
		if (mCamera->getSceneMode() <= 1) {
			ExifInfo.meteringMode = mCamera->getMetering();
			if (mCamera->getWhiteBalance() <= 1) {
				ExifInfo.whiteBalance = 0;
			} else {
				ExifInfo.whiteBalance = 1;
			}
			ExifInfo.saturation =
				convertToExifLMH(mCamera->getSaturation(),
						 2);
			ExifInfo.sharpness =
				convertToExifLMH(mCamera->getSharpness(),
						 2);
			switch (mCamera->getISO()) {
			case 2:
				ExifInfo.isoSpeedRating = 50;
				break;
			case 3:
				ExifInfo.isoSpeedRating = 100;
				break;
			case 4:
				ExifInfo.isoSpeedRating = 200;
				break;
			case 5:
				ExifInfo.isoSpeedRating = 400;
				break;
			case 6:
				ExifInfo.isoSpeedRating = 800;
				break;
			default:
				ExifInfo.isoSpeedRating = tempISO;
				break;
			}

			switch (mCamera->getBrightness()) {
			case 0:
				ExifInfo.exposureBias.numerator = -20;
				break;
			case 1:
				ExifInfo.exposureBias.numerator = -15;
				break;
			case 2:
				ExifInfo.exposureBias.numerator = -10;
				break;
			case 3:
				ExifInfo.exposureBias.numerator = -5;
				break;
			case 4:
				ExifInfo.exposureBias.numerator = 0;
				break;
			case 5:
				ExifInfo.exposureBias.numerator = 5;
				break;
			case 6:
				ExifInfo.exposureBias.numerator = 10;
				break;
			case 7:
				ExifInfo.exposureBias.numerator = 15;
				break;
			case 8:
				ExifInfo.exposureBias.numerator = 20;
				break;
			default:
				ExifInfo.exposureBias.numerator = 0;
				break;
			}
			ExifInfo.exposureBias.denominator = 10;
			ExifInfo.sceneCaptureType = 0;
		} else {
			switch (mCamera->getSceneMode()) {
			case 3:	//sunset
				ExifInfo.meteringMode = 2;
				ExifInfo.whiteBalance = 1;
				ExifInfo.saturation = 0;
				ExifInfo.sharpness = 0;
				ExifInfo.isoSpeedRating = tempISO;
				ExifInfo.exposureBias.numerator = 0;
				ExifInfo.exposureBias.denominator = 10;
				ExifInfo.sceneCaptureType = 4;
				break;
			case 4:	//dawn
				ExifInfo.meteringMode = 2;
				ExifInfo.whiteBalance = 1;
				ExifInfo.saturation = 0;
				ExifInfo.sharpness = 0;
				ExifInfo.isoSpeedRating = tempISO;
				ExifInfo.exposureBias.numerator = 0;
				ExifInfo.exposureBias.denominator = 10;
				ExifInfo.sceneCaptureType = 4;
				break;
			case 5:	//candlelight
				ExifInfo.meteringMode = 2;
				ExifInfo.whiteBalance = 1;
				ExifInfo.saturation = 0;
				ExifInfo.sharpness = 0;
				ExifInfo.isoSpeedRating = tempISO;
				ExifInfo.exposureBias.numerator = 0;
				ExifInfo.exposureBias.denominator = 10;
				ExifInfo.sceneCaptureType = 4;
				break;
			case 6:	//beach & snow
				ExifInfo.meteringMode = 2;
				ExifInfo.whiteBalance = 0;
				ExifInfo.saturation = 2;
				ExifInfo.sharpness = 0;
				ExifInfo.isoSpeedRating = 50;
				ExifInfo.exposureBias.numerator = 10;
				ExifInfo.exposureBias.denominator = 10;
				ExifInfo.sceneCaptureType = 1;
				break;
			case 7:	//againstlight
				ExifInfo.whiteBalance = 0;
				ExifInfo.saturation = 0;
				ExifInfo.sharpness = 0;
				ExifInfo.isoSpeedRating = tempISO;
				if (mPreviousFlashMode <= 1) {
					ExifInfo.meteringMode = 3;
				} else {
					ExifInfo.meteringMode = 2;
				}
				ExifInfo.exposureBias.numerator = 0;
				ExifInfo.exposureBias.denominator = 10;
				ExifInfo.sceneCaptureType = 4;
				break;
			case 8:	//text
				ExifInfo.meteringMode = 2;
				ExifInfo.whiteBalance = 0;
				ExifInfo.saturation = 0;
				ExifInfo.sharpness = 2;
				ExifInfo.isoSpeedRating = tempISO;
				ExifInfo.exposureBias.numerator = 0;
				ExifInfo.exposureBias.denominator = 10;
				ExifInfo.sceneCaptureType = 4;
				break;
			case 9:	//night
				ExifInfo.meteringMode = 2;
				ExifInfo.whiteBalance = 0;
				ExifInfo.saturation = 0;
				ExifInfo.sharpness = 0;
				ExifInfo.isoSpeedRating = tempISO;
				ExifInfo.exposureBias.numerator = 0;
				ExifInfo.exposureBias.denominator = 10;
				ExifInfo.sceneCaptureType = 3;
				break;
			case 10:	//landscape
				ExifInfo.meteringMode = 5;
				ExifInfo.whiteBalance = 0;
				ExifInfo.saturation = 2;
				ExifInfo.sharpness = 2;
				ExifInfo.isoSpeedRating = tempISO;
				ExifInfo.exposureBias.numerator = 0;
				ExifInfo.exposureBias.denominator = 10;
				ExifInfo.sceneCaptureType = 1;
				break;
			case 11:	//fireworks
				ExifInfo.meteringMode = 2;
				ExifInfo.whiteBalance = 0;
				ExifInfo.saturation = 0;
				ExifInfo.sharpness = 0;
				ExifInfo.isoSpeedRating = 50;
				ExifInfo.exposureBias.numerator = 0;
				ExifInfo.exposureBias.denominator = 10;
				ExifInfo.sceneCaptureType = 4;
				break;
			case 12:	//portrait
				ExifInfo.meteringMode = 2;
				ExifInfo.whiteBalance = 0;
				ExifInfo.saturation = 0;
				ExifInfo.sharpness = 1;
				ExifInfo.isoSpeedRating = tempISO;
				ExifInfo.exposureBias.numerator = 0;
				ExifInfo.exposureBias.denominator = 10;
				ExifInfo.sceneCaptureType = 2;
				break;
			case 13:	//fallcolor
				ExifInfo.meteringMode = 2;
				ExifInfo.whiteBalance = 0;
				ExifInfo.saturation = 2;
				ExifInfo.sharpness = 0;
				ExifInfo.isoSpeedRating = tempISO;
				ExifInfo.exposureBias.numerator = 0;
				ExifInfo.exposureBias.denominator = 10;
				ExifInfo.sceneCaptureType = 4;
				break;
			case 14:	//indoors
				ExifInfo.meteringMode = 2;
				ExifInfo.whiteBalance = 0;
				ExifInfo.saturation = 2;
				ExifInfo.sharpness = 0;
				ExifInfo.isoSpeedRating = 200;
				ExifInfo.exposureBias.numerator = 0;
				ExifInfo.exposureBias.denominator = 10;
				ExifInfo.sceneCaptureType = 4;
				break;
			case 15:	//sports
				ExifInfo.meteringMode = 2;
				ExifInfo.whiteBalance = 0;
				ExifInfo.saturation = 0;
				ExifInfo.sharpness = 0;
				ExifInfo.isoSpeedRating = tempISO;
				ExifInfo.exposureBias.numerator = 0;
				ExifInfo.exposureBias.denominator = 10;
				ExifInfo.sceneCaptureType = 4;
				break;
			}
		}
	} else		// VGA Camera
	{
		if (mThumbnailWidth > 0 && mThumbnailHeight > 0) {
			//thumb nail data added here 
			ExifInfo.thumbStream = pInThumbnailData;
			ExifInfo.thumbSize = Inthumbsize;
			ExifInfo.thumbImageWidth = mThumbnailWidth;
			ExifInfo.thumbImageHeight = mThumbnailHeight;
			ExifInfo.hasThumbnail = true;
		} else {
			ExifInfo.hasThumbnail = false;
		}
		ExifInfo.exposureProgram = 3;
		ExifInfo.exposureMode = 0;
		ExifInfo.contrast = 0;
		ExifInfo.fNumber.numerator = 28;
		ExifInfo.fNumber.denominator = 10;
		ExifInfo.aperture.numerator = 26;
		ExifInfo.aperture.denominator = 10;
		ExifInfo.maxAperture.numerator = 26;
		ExifInfo.maxAperture.denominator = 10;
		ExifInfo.focalLength.numerator = 900;
		ExifInfo.focalLength.denominator = 1000;
		ExifInfo.shutterSpeed.numerator = 16;
		ExifInfo.shutterSpeed.denominator = 1;
		ExifInfo.brightness.numerator = 5;
		ExifInfo.brightness.denominator = 9;
		ExifInfo.iso = 1;

		switch (orientationValue) {
		case 0:
			ExifInfo.orientation = 1;
			break;
		case 90:
			ExifInfo.orientation = 6;
			break;
		case 180:
			ExifInfo.orientation = 3;
			break;
		case 270:
			ExifInfo.orientation = 8;
			break;
		default:
			ExifInfo.orientation = 1;
			break;
		}

		ExifInfo.meteringMode = mCamera->getMetering();
		ExifInfo.whiteBalance = 0;
		ExifInfo.saturation = 0;
		ExifInfo.sharpness = 0;
		ExifInfo.isoSpeedRating = 100;
		ExifInfo.exposureTime.numerator = 1;
		ExifInfo.exposureTime.denominator = 16;
		ExifInfo.flash = 0;
		switch (mCamera->getBrightness()) {
		case 0:
			ExifInfo.exposureBias.numerator = -20;
			break;
		case 1:
			ExifInfo.exposureBias.numerator = -15;
			break;
		case 2:
			ExifInfo.exposureBias.numerator = -10;
			break;
		case 3:
			ExifInfo.exposureBias.numerator = -5;
			break;
		case 4:
			ExifInfo.exposureBias.numerator = 0;
			break;
		case 5:
			ExifInfo.exposureBias.numerator = 5;
			break;
		case 6:
			ExifInfo.exposureBias.numerator = 10;
			break;
		case 7:
			ExifInfo.exposureBias.numerator = 15;
			break;
		case 8:
			ExifInfo.exposureBias.numerator = 20;
			break;
		default:
			ExifInfo.exposureBias.numerator = 0;
			break;
		}
		ExifInfo.exposureBias.denominator = 10;
		ExifInfo.sceneCaptureType = 0;
	}

	double arg0, arg3;
	int arg1, arg2;

	if (mParameters.get(mParameters.KEY_GPS_LATITUDE) != 0
		&& mParameters.get(mParameters.KEY_GPS_LONGITUDE) != 0) {
		arg0 = getGPSLatitude();

		if (arg0 > 0)
			ExifInfo.GPSLatitudeRef[0] = 'N';
		else
			ExifInfo.GPSLatitudeRef[0] = 'S';

		convertFromDecimalToGPSFormat(fabs(arg0), arg1, arg2,
						  arg3);

		ExifInfo.GPSLatitude[0].numerator = arg1;
		ExifInfo.GPSLatitude[0].denominator = 1;
		ExifInfo.GPSLatitude[1].numerator = arg2;
		ExifInfo.GPSLatitude[1].denominator = 1;
		ExifInfo.GPSLatitude[2].numerator = arg3;
		ExifInfo.GPSLatitude[2].denominator = 1;

		arg0 = getGPSLongitude();

		if (arg0 > 0)
			ExifInfo.GPSLongitudeRef[0] = 'E';
		else
			ExifInfo.GPSLongitudeRef[0] = 'W';

		convertFromDecimalToGPSFormat(fabs(arg0), arg1, arg2,
						  arg3);

		ExifInfo.GPSLongitude[0].numerator = arg1;
		ExifInfo.GPSLongitude[0].denominator = 1;
		ExifInfo.GPSLongitude[1].numerator = arg2;
		ExifInfo.GPSLongitude[1].denominator = 1;
		ExifInfo.GPSLongitude[2].numerator = arg3;
		ExifInfo.GPSLongitude[2].denominator = 1;

		arg0 = getGPSAltitude();

		if (arg0 > 0)
			ExifInfo.GPSAltitudeRef = 0;
		else
			ExifInfo.GPSAltitudeRef = 1;

		ExifInfo.GPSAltitude[0].numerator = fabs(arg0);
		ExifInfo.GPSAltitude[0].denominator = 1;

		//GPS_Time_Stamp
		ExifInfo.GPSTimestamp[0].numerator =
			(uint32_t) m_gpsHour;
		ExifInfo.GPSTimestamp[0].denominator = 1;
		ExifInfo.GPSTimestamp[1].numerator =
			(uint32_t) m_gpsMin;
		ExifInfo.GPSTimestamp[1].denominator = 1;
		ExifInfo.GPSTimestamp[2].numerator =
			(uint32_t) m_gpsSec;
		ExifInfo.GPSTimestamp[2].denominator = 1;

		//GPS_ProcessingMethod
		strcpy((char *)ExifInfo.GPSProcessingMethod,
			   mPreviousGPSProcessingMethod);

		//GPS_Date_Stamp
		strcpy((char *)ExifInfo.GPSDatestamp, m_gps_date);

		ExifInfo.hasGps = true;
	} else {
		ExifInfo.hasGps = false;
	}

	ExifSize =
		mExifCreator->
		ExifCreate_wo_GPS((unsigned char *)pOutExifBuf, &ExifInfo,
				  flag);
	OutExifSize = ExifSize;
	delete mExifCreator;
}
void CameraHardware::convertFromDecimalToGPSFormat(double arg1,
						   int &arg2, int &arg3,
						   double &arg4) {
	double temp1 = 0, temp2 = 0, temp3 = 0;
	arg2 = (int)arg1;
	temp1 = arg1 - arg2;
	temp2 = temp1 * 60;
	arg3 = (int)temp2;
	temp3 = temp2 - arg3;
	arg4 = temp3 * 60;
}
int CameraHardware::convertToExifLMH(int value, int key) {
	const int NORMAL = 0;
	const int LOW = 1;
	const int HIGH = 2;

	value -= key;

	if (value == 0)
		return NORMAL;
	if (value < 0)
		return LOW;
	else
		return HIGH;
}
double CameraHardware::getGPSLatitude() const {
	double gpsLatitudeValue = 0;
	if (mParameters.get(mParameters.KEY_GPS_LATITUDE)) {
		gpsLatitudeValue =
			atof(mParameters.get(mParameters.KEY_GPS_LATITUDE));
		if (gpsLatitudeValue != 0) {
			LOGD("getGPSLatitude = %2.2f \n",
				 gpsLatitudeValue);
		}
		return gpsLatitudeValue;
	} else {
		LOGD("getGPSLatitude null \n");
		return 0;
	}
}

double CameraHardware::getGPSLongitude() const {
	double gpsLongitudeValue = 0;
	if (mParameters.get(mParameters.KEY_GPS_LONGITUDE)) {
		gpsLongitudeValue =
			atof(mParameters.
			 get(mParameters.KEY_GPS_LONGITUDE));
		if (gpsLongitudeValue != 0) {
			LOGD("getGPSLongitude = %2.2f \n",
				 gpsLongitudeValue);
		}
		return gpsLongitudeValue;
	} else {
		LOGD("getGPSLongitude null \n");
		return 0;
	}
}

double CameraHardware::getGPSAltitude() const {
	double gpsAltitudeValue = 0;
	if (mParameters.get(mParameters.KEY_GPS_ALTITUDE)) {
		gpsAltitudeValue =
			atof(mParameters.get(mParameters.KEY_GPS_ALTITUDE));
		if (gpsAltitudeValue != 0) {
			LOGD("getGPSAltitude = %2.2f \n",
				 gpsAltitudeValue);
		}
		return gpsAltitudeValue;
	} else {
		LOGD("getGPSAltitude null \n");
		return 0;
	}
}

void CameraHardware::setSkipFrame(int frame) {
	Mutex::Autolock lock(mSkipFrameLock);
	if (frame < mSkipFrame)
		return;

	mSkipFrame = frame;
}
};				// namespace android
