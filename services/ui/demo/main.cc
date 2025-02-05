// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/c/system/main.h"
#include "services/shell/public/cpp/application_runner.h"
#include "services/ui/demo/mus_demo.h"

MojoResult MojoMain(MojoHandle shell_handle) {
  shell::ApplicationRunner runner(new ui::demo::MusDemo);
  runner.set_message_loop_type(base::MessageLoop::TYPE_UI);
  return runner.Run(shell_handle);
}
