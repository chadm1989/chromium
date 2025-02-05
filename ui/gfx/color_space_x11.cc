// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

extern "C" {
#include <X11/Xatom.h>
#include <X11/Xlib.h>
}

#include "ui/gfx/color_space.h"

#include "ui/gfx/x/x11_types.h"

namespace gfx {

// static
ColorSpace ColorSpace::FromBestMonitor() {
  Atom property = XInternAtom(GetXDisplay(), "_ICC_PROFILE", true);
  if (property != None) {
    Atom prop_type = None;
    int prop_format = 0;
    unsigned long nitems = 0;
    unsigned long nbytes = 0;
    char* property_data = NULL;
    if (XGetWindowProperty(
            GetXDisplay(), DefaultRootWindow(GetXDisplay()), property, 0,
            0x1FFFFFFF /* MAXINT32 / 4 */, False, AnyPropertyType, &prop_type,
            &prop_format, &nitems, &nbytes,
            reinterpret_cast<unsigned char**>(&property_data)) == Success) {
      std::vector<char> icc_profile;
      icc_profile.assign(property_data, property_data + nitems);
      XFree(property_data);
      return FromICCProfile(icc_profile);
    }
  }
  return ColorSpace();
}

}  // namespace gfx
