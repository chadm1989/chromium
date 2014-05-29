// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Implementation of a VideoCaptureDeviceFactory class for Mac.

#ifndef MEDIA_VIDEO_CAPTURE_VIDEO_CAPTURE_DEVICE_FACTORY_MAC_H_
#define MEDIA_VIDEO_CAPTURE_VIDEO_CAPTURE_DEVICE_FACTORY_MAC_H_

#include "media/video/capture/video_capture_device_factory.h"

namespace media {

// Extension of VideoCaptureDeviceFactory to create and manipulate Mac devices.
class MEDIA_EXPORT VideoCaptureDeviceFactoryMac :
    public VideoCaptureDeviceFactory {
 public:
  VideoCaptureDeviceFactoryMac();
  virtual ~VideoCaptureDeviceFactoryMac() {}

  virtual scoped_ptr<VideoCaptureDevice> Create(
      scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner,
      const VideoCaptureDevice::Name& device_name) OVERRIDE;
  virtual void GetDeviceNames(VideoCaptureDevice::Names* device_names) OVERRIDE;
  virtual void GetDeviceSupportedFormats(
      const VideoCaptureDevice::Name& device,
      VideoCaptureFormats* supported_formats) OVERRIDE;

 private:
  DISALLOW_COPY_AND_ASSIGN(VideoCaptureDeviceFactoryMac);
};

}  // namespace media

#endif  // MEDIA_VIDEO_CAPTURE_VIDEO_CAPTURE_DEVICE_FACTORY_MAC_H_
