// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_WEB_VIEW_TEST_FRAME_TREE_DELEGATE_H_
#define COMPONENTS_WEB_VIEW_TEST_FRAME_TREE_DELEGATE_H_

#include "base/basictypes.h"
#include "base/memory/scoped_ptr.h"
#include "components/web_view/frame_tree_delegate.h"

namespace base {
class RunLoop;
}

namespace mojo {
class ApplicationImpl;
}

namespace web_view {

class TestFrameTreeDelegate : public FrameTreeDelegate {
 public:
  explicit TestFrameTreeDelegate(mojo::ApplicationImpl* app);
  ~TestFrameTreeDelegate() override;

  mojo::ApplicationImpl* app() { return app_; }

  // Runs a message loop until DidCreateFrame() is called, returning the
  // Frame supplied to DidCreateFrame().
  Frame* WaitForCreateFrame();

  // Wires for DidDestroyFrame() to be called with |frame|.
  void WaitForDestroyFrame(Frame* frame);

  // TestFrameTreeDelegate:
  bool CanPostMessageEventToFrame(const Frame* source,
                                  const Frame* target,
                                  HTMLMessageEvent* event) override;
  void LoadingStateChanged(bool loading) override;
  void ProgressChanged(double progress) override;
  void TitleChanged(const mojo::String& title) override;
  void NavigateTopLevel(Frame* source, mojo::URLRequestPtr request) override;
  void CanNavigateFrame(Frame* target,
                        mojo::URLRequestPtr request,
                        const CanNavigateFrameCallback& callback) override;
  void DidStartNavigation(Frame* frame) override;
  void DidCreateFrame(Frame* frame) override;
  void DidDestroyFrame(Frame* frame) override;

 private:
  bool is_waiting() const { return run_loop_.get(); }

  mojo::ApplicationImpl* app_;
  bool waiting_for_create_frame_;
  Frame* waiting_for_destroy_frame_;
  scoped_ptr<base::RunLoop> run_loop_;
  Frame* most_recent_frame_;

  DISALLOW_COPY_AND_ASSIGN(TestFrameTreeDelegate);
};

}  // namespace web_view

#endif  // COMPONENTS_WEB_VIEW_TEST_FRAME_TREE_DELEGATE_H_
