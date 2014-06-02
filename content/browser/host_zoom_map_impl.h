// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_HOST_ZOOM_MAP_IMPL_H_
#define CONTENT_BROWSER_HOST_ZOOM_MAP_IMPL_H_

#include <map>
#include <string>
#include <vector>

#include "base/compiler_specific.h"
#include "base/sequenced_task_runner_helpers.h"
#include "base/supports_user_data.h"
#include "base/synchronization/lock.h"
#include "content/public/browser/host_zoom_map.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"

namespace content {

class WebContentsImpl;

// HostZoomMap needs to be deleted on the UI thread because it listens
// to notifications on there (and holds a NotificationRegistrar).
class CONTENT_EXPORT HostZoomMapImpl : public NON_EXPORTED_BASE(HostZoomMap),
                                       public NotificationObserver,
                                       public base::SupportsUserData::Data {
 public:
  HostZoomMapImpl();
  virtual ~HostZoomMapImpl();

  // HostZoomMap implementation:
  virtual void CopyFrom(HostZoomMap* copy) OVERRIDE;
  virtual double GetZoomLevelForHostAndScheme(
      const std::string& scheme,
      const std::string& host) const OVERRIDE;
  virtual ZoomLevelVector GetAllZoomLevels() const OVERRIDE;
  virtual void SetZoomLevelForHost(
      const std::string& host,
      double level) OVERRIDE;
  virtual void SetZoomLevelForHostAndScheme(
      const std::string& scheme,
      const std::string& host,
      double level) OVERRIDE;
  virtual double GetDefaultZoomLevel() const OVERRIDE;
  virtual void SetDefaultZoomLevel(double level) OVERRIDE;
  virtual scoped_ptr<Subscription> AddZoomLevelChangedCallback(
      const ZoomLevelChangedCallback& callback) OVERRIDE;

  // Returns the current zoom level for the specified WebContents. This may
  // be a temporary zoom level, depending on UsesTemporaryZoomLevel().
  double GetZoomLevelForWebContents(
      const WebContentsImpl& web_contents_impl) const;

  // Sets the zoom level for this WebContents. If this WebContents is using
  // a temporary zoom level, then level is only applied to this WebContents.
  // Otherwise, the level will be applied on a host level.
  void SetZoomLevelForWebContents(const WebContentsImpl& web_contents_impl,
                                  double level);

  // Sets the zoom level for the specified view. The level may be set for only
  // this view, or for the host, depending on UsesTemporaryZoomLevel().
  void SetZoomLevelForView(int render_process_id,
                           int render_view_id,
                           double level,
                           const std::string& host);

  // Returns whether the view manages its zoom level independently of other tabs
  // displaying content from the same host.
  bool UsesTemporaryZoomLevel(int render_process_id, int render_view_id) const;

  // Sets whether the view manages its zoom level independently of other tabs
  // displaying content from the same host, based on whether
  // |uses_temporary_zoom_level| is true.
  void SetUsesTemporaryZoomLevel(int render_process_id,
                                 int render_view_id,
                                 bool uses_temporary_zoom_level);

  // Returns the temporary zoom level that's only valid for the lifetime of
  // the given WebContents (i.e. isn't saved and doesn't affect other
  // WebContentses) if it exists, the default zoom level otherwise.
  //
  // This may be called on any thread.
  double GetTemporaryZoomLevel(int render_process_id,
                               int render_view_id) const;

  // Sets the temporary zoom level that's only valid for the lifetime of this
  // WebContents.
  //
  // This should only be called on the UI thread.
  void SetTemporaryZoomLevel(int render_process_id,
                             int render_view_id,
                             double level);

  // NotificationObserver implementation.
  virtual void Observe(int type,
                       const NotificationSource& source,
                       const NotificationDetails& details) OVERRIDE;

 private:
  typedef std::map<std::string, double> HostZoomLevels;
  typedef std::map<std::string, HostZoomLevels> SchemeHostZoomLevels;

  struct TemporaryZoomLevel {
    TemporaryZoomLevel(int process_id, int view_id, double level);
    TemporaryZoomLevel(int process_id, int view_id);
    bool operator==(const TemporaryZoomLevel& other) const;

    int render_process_id;
    int render_view_id;
    double zoom_level;
  };

  typedef std::vector<TemporaryZoomLevel> TemporaryZoomLevels;

  double GetZoomLevelForHost(const std::string& host) const;

  // Callbacks called when zoom level changes.
  base::CallbackList<void(const ZoomLevelChange&)>
      zoom_level_changed_callbacks_;

  // Copy of the pref data, so that we can read it on the IO thread.
  HostZoomLevels host_zoom_levels_;
  SchemeHostZoomLevels scheme_host_zoom_levels_;
  double default_zoom_level_;

  // Don't expect more than a couple of tabs that are using a temporary zoom
  // level, so vector is fine for now.
  TemporaryZoomLevels temporary_zoom_levels_;

  // Used around accesses to |host_zoom_levels_|, |default_zoom_level_| and
  // |temporary_zoom_levels_| to guarantee thread safety.
  mutable base::Lock lock_;

  NotificationRegistrar registrar_;

  DISALLOW_COPY_AND_ASSIGN(HostZoomMapImpl);
};

}  // namespace content

#endif  // CONTENT_BROWSER_HOST_ZOOM_MAP_IMPL_H_
