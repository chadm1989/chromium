/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "modules/notifications/NotificationController.h"

#include "modules/notifications/NotificationClient.h"
#include "wtf/PassOwnPtr.h"

namespace WebCore {

NotificationController::NotificationController(PassOwnPtr<NotificationClient> client)
    : m_client(client)
{
}

NotificationController::~NotificationController()
{
}

PassOwnPtr<NotificationController> NotificationController::create(PassOwnPtr<NotificationClient> client)
{
    return adoptPtr(new NotificationController(client));
}

NotificationClient* NotificationController::clientFrom(LocalFrame* frame)
{
    if (NotificationController* controller = NotificationController::from(frame))
        return controller->client();
    return 0;
}

const char* NotificationController::supplementName()
{
    return "NotificationController";
}

void provideNotification(LocalFrame& frame, PassOwnPtr<NotificationClient> client)
{
    NotificationController::provideTo(frame, NotificationController::supplementName(), NotificationController::create(client));
}

} // namespace WebCore
