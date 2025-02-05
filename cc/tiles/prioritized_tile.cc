// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/tiles/prioritized_tile.h"

#include "cc/debug/traced_value.h"
#include "cc/tiles/picture_layer_tiling.h"

namespace cc {

PrioritizedTile::PrioritizedTile() : tile_(nullptr), is_occluded_(false) {}

PrioritizedTile::PrioritizedTile(Tile* tile,
                                 scoped_refptr<RasterSource> raster_source,
                                 const TilePriority priority,
                                 bool is_occluded)
    : tile_(tile),
      raster_source_(std::move(raster_source)),
      priority_(priority),
      is_occluded_(is_occluded) {}

PrioritizedTile::PrioritizedTile(const PrioritizedTile& other) = default;
PrioritizedTile::PrioritizedTile(PrioritizedTile&& other) = default;
PrioritizedTile::~PrioritizedTile() = default;

PrioritizedTile& PrioritizedTile::operator=(const PrioritizedTile& other) =
    default;
PrioritizedTile& PrioritizedTile::operator=(PrioritizedTile&& other) = default;

void PrioritizedTile::AsValueInto(base::trace_event::TracedValue* value) const {
  tile_->AsValueInto(value);

  TracedValue::SetIDRef(raster_source().get(), value, "picture_pile");

  value->BeginDictionary("combined_priority");
  priority().AsValueInto(value);
  value->SetBoolean("is_occluded", is_occluded_);
  value->EndDictionary();

  value->SetString("resolution", TileResolutionToString(priority().resolution));
}

}  // namespace cc
