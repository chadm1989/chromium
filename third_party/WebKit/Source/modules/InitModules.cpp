// Copyright (c) 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "config.h"
#include "InitModules.h"

#include "EventModulesNames.h"
#include "EventTargetModulesNames.h"
#include "EventTypeNames.h"

namespace WebCore {

void ModulesInitializer::initEventNames()
{
    EventNames::init();
    EventNames::initModules();
}

void ModulesInitializer::initEventTargetNames()
{
    EventTargetNames::init();
    EventTargetNames::initModules();
}

} // namespace WebCore
