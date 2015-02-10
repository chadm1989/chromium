// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_DRI_NATIVE_DISPLAY_DELEGATE_DRI_H_
#define UI_OZONE_PLATFORM_DRI_NATIVE_DISPLAY_DELEGATE_DRI_H_

#include "base/memory/scoped_ptr.h"
#include "base/memory/scoped_vector.h"
#include "base/observer_list.h"
#include "ui/display/types/native_display_delegate.h"

namespace ui {

class DeviceManager;
class DisplaySnapshotDri;
class DriWrapper;
class ScreenManager;

class NativeDisplayDelegateDri : public NativeDisplayDelegate {
 public:
  NativeDisplayDelegateDri(const scoped_refptr<DriWrapper>& dri,
                           ScreenManager* screen_manager);
  ~NativeDisplayDelegateDri() override;

  DisplaySnapshot* FindDisplaySnapshot(int64_t id);
  const DisplayMode* FindDisplayMode(const gfx::Size& size,
                                     bool is_interlaced,
                                     float refresh_rate);

  std::vector<DisplaySnapshot*> GetDisplays();
  bool Configure(const DisplaySnapshot& output,
                 const DisplayMode* mode,
                 const gfx::Point& origin);

  // NativeDisplayDelegate overrides:
  void Initialize() override;
  void GrabServer() override;
  void UngrabServer() override;
  bool TakeDisplayControl() override;
  bool RelinquishDisplayControl() override;
  void SyncWithServer() override;
  void SetBackgroundColor(uint32_t color_argb) override;
  void ForceDPMSOn() override;
  void GetDisplays(const GetDisplaysCallback& callback) override;
  void AddMode(const DisplaySnapshot& output, const DisplayMode* mode) override;
  void Configure(const DisplaySnapshot& output,
                 const DisplayMode* mode,
                 const gfx::Point& origin,
                 const ConfigureCallback& callback) override;
  void CreateFrameBuffer(const gfx::Size& size) override;
  bool GetHDCPState(const DisplaySnapshot& output, HDCPState* state) override;
  bool SetHDCPState(const DisplaySnapshot& output, HDCPState state) override;
  std::vector<ui::ColorCalibrationProfile> GetAvailableColorCalibrationProfiles(
      const ui::DisplaySnapshot& output) override;
  bool SetColorCalibrationProfile(
      const ui::DisplaySnapshot& output,
      ui::ColorCalibrationProfile new_profile) override;
  void AddObserver(NativeDisplayObserver* observer) override;
  void RemoveObserver(NativeDisplayObserver* observer) override;

 private:
  // Notify ScreenManager of all the displays that were present before the
  // update but are gone after the update.
  void NotifyScreenManager(
      const std::vector<DisplaySnapshotDri*>& new_displays,
      const std::vector<DisplaySnapshotDri*>& old_displays) const;

  scoped_refptr<DriWrapper> dri_;
  ScreenManager* screen_manager_;  // Not owned.
  // Modes can be shared between different displays, so we need to keep track
  // of them independently for cleanup.
  ScopedVector<const DisplayMode> cached_modes_;
  ScopedVector<DisplaySnapshotDri> cached_displays_;
  ObserverList<NativeDisplayObserver> observers_;

  DISALLOW_COPY_AND_ASSIGN(NativeDisplayDelegateDri);
};

}  // namespace ui

#endif  // UI_OZONE_PLATFORM_DRI_NATIVE_DISPLAY_DELEGATE_DRI_H_
