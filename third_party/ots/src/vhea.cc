// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "vhea.h"

#include "gsub.h"
#include "head.h"
#include "maxp.h"

// vhea - Vertical Header Table
// http://www.microsoft.com/typography/otspec/vhea.htm

#define TABLE_NAME "vhea"

namespace ots {

bool ots_vhea_parse(Font *font, const uint8_t *data, size_t length) {
  Buffer table(data, length);
  OpenTypeVHEA *vhea = new OpenTypeVHEA;
  font->vhea = vhea;

  if (!table.ReadU32(&vhea->header.version)) {
    return OTS_FAILURE_MSG("Failed to read version");
  }
  if (vhea->header.version != 0x00010000 &&
      vhea->header.version != 0x00011000) {
    return OTS_FAILURE_MSG("Bad vhea version %x", vhea->header.version);
  }

  if (!ParseMetricsHeader(font, &table, &vhea->header)) {
    return OTS_FAILURE_MSG("Failed to parse metrics in vhea");
  }

  return true;
}

bool ots_vhea_should_serialise(Font *font) {
  // vhea should'nt serialise when vmtx doesn't exist.
  // Firefox developer pointed out that vhea/vmtx should serialise iff GSUB is
  // preserved. See http://crbug.com/77386
  return font->vhea != NULL && font->vmtx != NULL &&
      ots_gsub_should_serialise(font);
}

bool ots_vhea_serialise(OTSStream *out, Font *font) {
  if (!SerialiseMetricsHeader(font, out, &font->vhea->header)) {
    return OTS_FAILURE_MSG("Failed to write vhea metrics");
  }
  return true;
}

void ots_vhea_reuse(Font *font, Font *other) {
  font->vhea = other->vhea;
  font->vhea_reused = true;
}

void ots_vhea_free(Font *font) {
  delete font->vhea;
}

}  // namespace ots

#undef TABLE_NAME
