// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_MEDIA_ANDROID_BROWSER_MEDIA_PLAYER_MANAGER_H_
#define CONTENT_BROWSER_MEDIA_ANDROID_BROWSER_MEDIA_PLAYER_MANAGER_H_

#include "base/basictypes.h"
#include "base/callback.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/scoped_vector.h"
#include "base/time/time.h"
#include "content/browser/android/content_video_view.h"
#include "content/common/content_export.h"
#include "content/common/media/media_player_messages_enums_android.h"
#include "ipc/ipc_message.h"
#include "media/base/android/media_player_android.h"
#include "media/base/android/media_player_manager.h"
#include "media/base/android/media_url_interceptor.h"
#include "ui/gfx/geometry/rect_f.h"
#include "url/gurl.h"

namespace media {
class DemuxerAndroid;
}

struct MediaPlayerHostMsg_Initialize_Params;

namespace content {
class BrowserDemuxerAndroid;
class ContentViewCoreImpl;
class ExternalVideoSurfaceContainer;
class RenderFrameHost;
class WebContents;

// This class manages all the MediaPlayerAndroid objects.
// It receives control operations from the the render process, and forwards
// them to corresponding MediaPlayerAndroid object. Callbacks from
// MediaPlayerAndroid objects are converted to IPCs and then sent to the render
// process.
class CONTENT_EXPORT BrowserMediaPlayerManager
    : public media::MediaPlayerManager {
 public:
  // Permits embedders to provide an extended version of the class.
  typedef BrowserMediaPlayerManager* (*Factory)(RenderFrameHost*);
  static void RegisterFactory(Factory factory);

  // Permits embedders to handle custom urls.
  static void RegisterMediaUrlInterceptor(
      media::MediaUrlInterceptor* media_url_interceptor);

  // Returns a new instance using the registered factory if available.
  static BrowserMediaPlayerManager* Create(RenderFrameHost* rfh);

  ContentViewCoreImpl* GetContentViewCore() const;

  virtual ~BrowserMediaPlayerManager();

  // Fullscreen video playback controls.
  virtual void ExitFullscreen(bool release_media_player);
  virtual void SetVideoSurface(gfx::ScopedJavaSurface surface);

  // Called when browser player wants the renderer media element to seek.
  // Any actual seek started by renderer will be handled by browser in OnSeek().
  void OnSeekRequest(int player_id, const base::TimeDelta& time_to_seek);

  // Stops and releases every media managed by this class.
  void ReleaseAllMediaPlayers();

  // media::MediaPlayerManager overrides.
  virtual void OnTimeUpdate(
      int player_id,
      base::TimeDelta current_timestamp,
      base::TimeTicks current_time_ticks) override;
  virtual void OnMediaMetadataChanged(
      int player_id,
      base::TimeDelta duration,
      int width,
      int height,
      bool success) override;
  virtual void OnPlaybackComplete(int player_id) override;
  virtual void OnMediaInterrupted(int player_id) override;
  virtual void OnBufferingUpdate(int player_id, int percentage) override;
  virtual void OnSeekComplete(
      int player_id,
      const base::TimeDelta& current_time) override;
  virtual void OnError(int player_id, int error) override;
  virtual void OnVideoSizeChanged(
      int player_id, int width, int height) override;
  virtual media::MediaResourceGetter* GetMediaResourceGetter() override;
  virtual media::MediaUrlInterceptor* GetMediaUrlInterceptor() override;
  virtual media::MediaPlayerAndroid* GetFullscreenPlayer() override;
  virtual media::MediaPlayerAndroid* GetPlayer(int player_id) override;
  virtual void RequestFullScreen(int player_id) override;
#if defined(VIDEO_HOLE)
  virtual bool ShouldUseVideoOverlayForEmbeddedEncryptedVideo() override;

  void AttachExternalVideoSurface(int player_id, jobject surface);
  void DetachExternalVideoSurface(int player_id);
  void OnFrameInfoUpdated();
#endif  // defined(VIDEO_HOLE)

  // Message handlers.
  virtual void OnEnterFullscreen(int player_id);
  virtual void OnExitFullscreen(int player_id);
  virtual void OnInitialize(
      const MediaPlayerHostMsg_Initialize_Params& media_player_params);
  virtual void OnStart(int player_id);
  virtual void OnSeek(int player_id, const base::TimeDelta& time);
  virtual void OnPause(int player_id, bool is_media_related_action);
  virtual void OnSetVolume(int player_id, double volume);
  virtual void OnSetPoster(int player_id, const GURL& poster);
  virtual void OnReleaseResources(int player_id);
  virtual void OnDestroyPlayer(int player_id);
  virtual void OnRequestRemotePlayback(int player_id);
  virtual void OnRequestRemotePlaybackControl(int player_id);
  virtual void ReleaseFullscreenPlayer(media::MediaPlayerAndroid* player);
#if defined(VIDEO_HOLE)
  void OnNotifyExternalSurface(
      int player_id, bool is_request, const gfx::RectF& rect);
#endif  // defined(VIDEO_HOLE)

 protected:
  // Clients must use Create() or subclass constructor.
  explicit BrowserMediaPlayerManager(RenderFrameHost* render_frame_host);

  WebContents* web_contents() const { return web_contents_; }

  // Adds a given player to the list.
  void AddPlayer(media::MediaPlayerAndroid* player);

  // Removes the player with the specified id.
  void RemovePlayer(int player_id);

  // Replaces a player with the specified id with a given MediaPlayerAndroid
  // object. This will also return the original MediaPlayerAndroid object that
  // was replaced.
  scoped_ptr<media::MediaPlayerAndroid> SwapPlayer(
      int player_id,
      media::MediaPlayerAndroid* player);

  int RoutingID();

  // Helper function to send messages to RenderFrameObserver.
  bool Send(IPC::Message* msg);

 private:
  // Constructs a MediaPlayerAndroid object.
  media::MediaPlayerAndroid* CreateMediaPlayer(
      const MediaPlayerHostMsg_Initialize_Params& media_player_params,
      bool hide_url_log,
      media::MediaPlayerManager* manager,
      BrowserDemuxerAndroid* demuxer);

  // MediaPlayerAndroid must call this before it is going to decode
  // media streams. This helps the manager object maintain an array
  // of active MediaPlayerAndroid objects and release the resources
  // when needed. Currently we only count video resources as they are
  // constrained by hardware and memory limits.
  virtual void OnMediaResourcesRequested(int player_id);

  // Called when a player releases all decoding resources.
  void ReleaseMediaResources(int player_id);

  // Releases the player. However, don't remove it from |players_|.
  void ReleasePlayer(media::MediaPlayerAndroid* player);

#if defined(VIDEO_HOLE)
  void ReleasePlayerOfExternalVideoSurfaceIfNeeded(int future_player);
  void OnRequestExternalSurface(int player_id, const gfx::RectF& rect);
#endif  // defined(VIDEO_HOLE)

  RenderFrameHost* const render_frame_host_;

  // An array of managed players.
  ScopedVector<media::MediaPlayerAndroid> players_;

  // The fullscreen video view object or NULL if video is not played in
  // fullscreen.
  scoped_ptr<ContentVideoView> video_view_;

#if defined(VIDEO_HOLE)
  scoped_ptr<ExternalVideoSurfaceContainer> external_video_surface_container_;
#endif

  // Player ID of the fullscreen media player.
  int fullscreen_player_id_;

  // Whether the fullscreen player has been Release()-d.
  bool fullscreen_player_is_released_;

  WebContents* const web_contents_;

  // Object for retrieving resources media players.
  scoped_ptr<media::MediaResourceGetter> media_resource_getter_;

  // NOTE: Weak pointers must be invalidated before all other member variables.
  base::WeakPtrFactory<BrowserMediaPlayerManager> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(BrowserMediaPlayerManager);
};

}  // namespace content

#endif  // CONTENT_BROWSER_MEDIA_ANDROID_BROWSER_MEDIA_PLAYER_MANAGER_H_
