// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/web_view/test_frame_tree_delegate.h"

#include "base/run_loop.h"
#include "components/web_view/frame_connection.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace web_view {

TestFrameTreeDelegate::TestFrameTreeDelegate(mojo::ApplicationImpl* app)
    : app_(app),
      waiting_for_create_frame_(false),
      waiting_for_destroy_frame_(nullptr),
      most_recent_frame_(nullptr) {}

TestFrameTreeDelegate::~TestFrameTreeDelegate() {}

Frame* TestFrameTreeDelegate::WaitForCreateFrame() {
  if (is_waiting()) {
    ADD_FAILURE() << "already waiting";
    return nullptr;
  }
  waiting_for_create_frame_ = true;
  run_loop_.reset(new base::RunLoop);
  run_loop_->Run();
  run_loop_.reset();
  return most_recent_frame_;
}

void TestFrameTreeDelegate::WaitForDestroyFrame(Frame* frame) {
  ASSERT_FALSE(is_waiting());
  waiting_for_destroy_frame_ = frame;
  run_loop_.reset(new base::RunLoop);
  run_loop_->Run();
  run_loop_.reset();
}

bool TestFrameTreeDelegate::CanPostMessageEventToFrame(
    const Frame* source,
    const Frame* target,
    HTMLMessageEvent* event) {
  return true;
}

void TestFrameTreeDelegate::LoadingStateChanged(bool loading) {}

void TestFrameTreeDelegate::ProgressChanged(double progress) {}

void TestFrameTreeDelegate::TitleChanged(const mojo::String& title) {}

void TestFrameTreeDelegate::NavigateTopLevel(Frame* source,
                                             mojo::URLRequestPtr request) {}

void TestFrameTreeDelegate::CanNavigateFrame(
    Frame* target,
    mojo::URLRequestPtr request,
    const CanNavigateFrameCallback& callback) {
  FrameConnection::CreateConnectionForCanNavigateFrame(
      app_, target, request.Pass(), callback);
}

void TestFrameTreeDelegate::DidStartNavigation(Frame* frame) {}

void TestFrameTreeDelegate::DidCreateFrame(Frame* frame) {
  if (waiting_for_create_frame_) {
    most_recent_frame_ = frame;
    run_loop_->Quit();
  }
}

void TestFrameTreeDelegate::DidDestroyFrame(Frame* frame) {
  if (waiting_for_destroy_frame_ == frame) {
    waiting_for_destroy_frame_ = nullptr;
    run_loop_->Quit();
  }
}

}  // namespace web_view
