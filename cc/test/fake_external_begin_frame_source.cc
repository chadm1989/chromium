// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/test/fake_external_begin_frame_source.h"

#include "base/location.h"
#include "base/message_loop/message_loop.h"
#include "base/time/time.h"
#include "cc/test/begin_frame_args_test.h"

namespace cc {

FakeExternalBeginFrameSource::FakeExternalBeginFrameSource(double refresh_rate)
    : milliseconds_per_frame_(1000.0 / refresh_rate),
      is_ready_(false),
      weak_ptr_factory_(this) {
  DetachFromThread();
}

FakeExternalBeginFrameSource::~FakeExternalBeginFrameSource() {
  DCHECK(CalledOnValidThread());
}

void FakeExternalBeginFrameSource::SetClientReady() {
  DCHECK(CalledOnValidThread());
  is_ready_ = true;
}

void FakeExternalBeginFrameSource::OnNeedsBeginFramesChange(
    bool needs_begin_frames) {
  DCHECK(CalledOnValidThread());
  if (needs_begin_frames) {
    base::MessageLoop::current()->PostDelayedTask(
        FROM_HERE, base::Bind(&FakeExternalBeginFrameSource::TestOnBeginFrame,
                              weak_ptr_factory_.GetWeakPtr()),
        base::TimeDelta::FromMilliseconds(milliseconds_per_frame_));
  }
}

void FakeExternalBeginFrameSource::TestOnBeginFrame() {
  DCHECK(CalledOnValidThread());
  CallOnBeginFrame(CreateBeginFrameArgsForTesting(BEGINFRAME_FROM_HERE));
}

}  // namespace cc
