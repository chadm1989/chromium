// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_TEST_IMPL_SIDE_PAINTING_SETTINGS_H_
#define CC_TEST_IMPL_SIDE_PAINTING_SETTINGS_H_

#include "cc/trees/layer_tree_settings.h"

namespace cc {

class ImplSidePaintingSettings : public LayerTreeSettings {
 public:
  ImplSidePaintingSettings() {
    impl_side_painting = true;
  }

  explicit ImplSidePaintingSettings(int interest_rect_tiles) {
    impl_side_painting = true;
    max_tiles_for_interest_area = interest_rect_tiles;
  }
};

}  // namespace cc

#endif  // CC_TEST_IMPL_SIDE_PAINTING_SETTINGS_H_
