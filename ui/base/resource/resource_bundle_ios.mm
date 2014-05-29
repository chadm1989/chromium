// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/resource/resource_bundle.h"

#import <QuartzCore/QuartzCore.h>
#import <UIKit/UIKit.h>

#include "base/basictypes.h"
#include "base/file_util.h"
#include "base/files/file_path.h"
#include "base/mac/bundle_locations.h"
#include "base/mac/foundation_util.h"
#include "base/mac/scoped_nsobject.h"
#include "base/memory/ref_counted_memory.h"
#include "base/strings/sys_string_conversions.h"
#include "base/synchronization/lock.h"
#include "ui/base/resource/resource_handle.h"
#include "ui/gfx/image/image.h"

namespace ui {

namespace {

base::FilePath GetResourcesPakFilePath(NSString* name, NSString* mac_locale) {
  NSString *resource_path;
  if ([mac_locale length]) {
    resource_path = [base::mac::FrameworkBundle() pathForResource:name
                                                           ofType:@"pak"
                                                      inDirectory:@""
                                                  forLocalization:mac_locale];
  } else {
    resource_path = [base::mac::FrameworkBundle() pathForResource:name
                                                           ofType:@"pak"];
  }
  if (!resource_path) {
    // Return just the name of the pak file.
    return base::FilePath(base::SysNSStringToUTF8(name) + ".pak");
  }
  return base::FilePath([resource_path fileSystemRepresentation]);
}

}  // namespace

void ResourceBundle::LoadCommonResources() {
  AddDataPackFromPath(GetResourcesPakFilePath(@"chrome", nil),
                      ui::SCALE_FACTOR_NONE);

  if (IsScaleFactorSupported(SCALE_FACTOR_100P)) {
    AddDataPackFromPath(GetResourcesPakFilePath(@"chrome_100_percent", nil),
                        SCALE_FACTOR_100P);
  }

  if (IsScaleFactorSupported(SCALE_FACTOR_200P)) {
    AddDataPackFromPath(GetResourcesPakFilePath(@"chrome_200_percent", nil),
                        SCALE_FACTOR_200P);
  }
}

base::FilePath ResourceBundle::GetLocaleFilePath(const std::string& app_locale,
                                                 bool test_file_exists) {
  NSString* mac_locale = base::SysUTF8ToNSString(app_locale);

  // iOS uses "_" instead of "-", so swap to get a iOS-style value.
  mac_locale = [mac_locale stringByReplacingOccurrencesOfString:@"-"
                                                     withString:@"_"];

  // On disk, the "en_US" resources are just "en" (http://crbug.com/25578).
  if ([mac_locale isEqual:@"en_US"])
    mac_locale = @"en";

  base::FilePath locale_file_path =
      GetResourcesPakFilePath(@"locale", mac_locale);

  if (delegate_) {
    locale_file_path =
        delegate_->GetPathForLocalePack(locale_file_path, app_locale);
  }

  // Don't try to load empty values or values that are not absolute paths.
  if (locale_file_path.empty() || !locale_file_path.IsAbsolute())
    return base::FilePath();

  if (test_file_exists && !base::PathExists(locale_file_path))
    return base::FilePath();

  return locale_file_path;
}

gfx::Image& ResourceBundle::GetNativeImageNamed(int resource_id, ImageRTL rtl) {
  // Flipped images are not used on iOS.
  DCHECK_EQ(rtl, RTL_DISABLED);

  // Check to see if the image is already in the cache.
  {
    base::AutoLock lock(*images_and_fonts_lock_);
    ImageMap::iterator found = images_.find(resource_id);
    if (found != images_.end()) {
      return found->second;
    }
  }

  gfx::Image image;
  if (delegate_)
    image = delegate_->GetNativeImageNamed(resource_id, rtl);

  if (image.IsEmpty()) {
    // Load the raw data from the resource pack at the current supported scale
    // factor.  This code assumes that only one of the possible scale factors is
    // supported at runtime, based on the device resolution.
    ui::ScaleFactor scale_factor = GetMaxScaleFactor();

    scoped_refptr<base::RefCountedStaticMemory> data(
        LoadDataResourceBytesForScale(resource_id, scale_factor));

    if (!data.get()) {
      LOG(WARNING) << "Unable to load image with id " << resource_id;
      return GetEmptyImage();
    }

    // Create a data object from the raw bytes.
    base::scoped_nsobject<NSData> ns_data(
        [[NSData alloc] initWithBytes:data->front() length:data->size()]);

    bool is_fallback = PNGContainsFallbackMarker(data->front(), data->size());
    // Create the image from the data.
    CGFloat target_scale = ui::GetImageScale(scale_factor);
    CGFloat source_scale = is_fallback ? 1.0 : target_scale;
    base::scoped_nsobject<UIImage> ui_image(
        [[UIImage alloc] initWithData:ns_data scale:source_scale]);

    // If the image is a 1x fallback, scale it up to a full-size representation.
    if (is_fallback) {
      CGSize source_size = [ui_image size];
      CGSize target_size = CGSizeMake(source_size.width * target_scale,
                                      source_size.height * target_scale);
      base::ScopedCFTypeRef<CGColorSpaceRef> color_space(
          CGColorSpaceCreateDeviceRGB());
      base::ScopedCFTypeRef<CGContextRef> context(CGBitmapContextCreate(
          NULL,
          target_size.width,
          target_size.height,
          8,
          target_size.width * 4,
          color_space,
          kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Host));

      CGRect target_rect = CGRectMake(0, 0,
                                      target_size.width, target_size.height);
      CGContextSetBlendMode(context, kCGBlendModeCopy);
      CGContextDrawImage(context, target_rect, [ui_image CGImage]);

      base::ScopedCFTypeRef<CGImageRef> cg_image(
          CGBitmapContextCreateImage(context));
      ui_image.reset([[UIImage alloc] initWithCGImage:cg_image
                                                scale:target_scale
                                          orientation:UIImageOrientationUp]);
    }

    if (!ui_image.get()) {
      LOG(WARNING) << "Unable to load image with id " << resource_id;
      NOTREACHED();  // Want to assert in debug mode.
      return GetEmptyImage();
    }

    // The gfx::Image takes ownership.
    image = gfx::Image(ui_image.release());
  }

  base::AutoLock lock(*images_and_fonts_lock_);

  // Another thread raced the load and has already cached the image.
  if (images_.count(resource_id))
    return images_[resource_id];

  images_[resource_id] = image;
  return images_[resource_id];
}

}  // namespace ui
