// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_MEDIA_ANDROID_MEDIA_WEB_CONTENTS_OBSERVER_H_
#define CONTENT_BROWSER_MEDIA_ANDROID_MEDIA_WEB_CONTENTS_OBSERVER_H_

#include "base/compiler_specific.h"
#include "base/containers/scoped_ptr_hash_map.h"
#include "content/common/content_export.h"
#include "content/public/browser/web_contents_observer.h"

namespace content {

class BrowserMediaPlayerManager;
class RenderViewHost;

// This class manages all RenderFrame based media related managers at the
// browser side. It receives IPC messages from media RenderFrameObservers and
// forwards them to the corresponding managers. The managers are responsible
// for sending IPCs back to the RenderFrameObservers at the render side.
class CONTENT_EXPORT MediaWebContentsObserver : public WebContentsObserver {
 public:
  explicit MediaWebContentsObserver(RenderViewHost* render_view_host);
  virtual ~MediaWebContentsObserver();

  // WebContentsObserver implementations.
  virtual void RenderFrameDeleted(RenderFrameHost* render_frame_host) OVERRIDE;
  virtual bool OnMessageReceived(const IPC::Message& message,
                                 RenderFrameHost* render_frame_host) OVERRIDE;

  // Gets the media player manager associated with |render_frame_host|. Creates
  // a new one if it doesn't exist. The caller doesn't own the returned pointer.
  BrowserMediaPlayerManager* GetMediaPlayerManager(
      RenderFrameHost* render_frame_host);

  // Pauses all media player.
  void PauseVideo();

#if defined(VIDEO_HOLE)
  void OnFrameInfoUpdated();
#endif  // defined(VIDEO_HOLE)

 private:
  // Map from RenderFrameHost* to BrowserMediaPlayerManager.
  typedef base::ScopedPtrHashMap<uintptr_t, BrowserMediaPlayerManager>
      MediaPlayerManagerMap;
  MediaPlayerManagerMap media_player_managers_;

  DISALLOW_COPY_AND_ASSIGN(MediaWebContentsObserver);
};

}  // namespace content

#endif  // CONTENT_BROWSER_MEDIA_ANDROID_MEDIA_WEB_CONTENTS_OBSERVER_H_
