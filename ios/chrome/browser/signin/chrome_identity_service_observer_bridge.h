// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_SIGNIN_CHROME_IDENTITY_SERVICE_OBSERVER_BRIDGE_H_
#define IOS_CHROME_BROWSER_SIGNIN_CHROME_IDENTITY_SERVICE_OBSERVER_BRIDGE_H_

#include <Foundation/Foundation.h>

#include "base/macros.h"
#include "base/scoped_observer.h"
#include "ios/public/provider/chrome/browser/signin/chrome_identity_service.h"

// Objective-C protocol mirroring ChromeIdentityService::Observer.
@protocol ChromeIdentityServiceObserver<NSObject>
@optional
- (void)onIdentityListChanged;
- (void)onAccessTokenRefreshFailed:(ChromeIdentity*)identity
                             error:(ios::AccessTokenErrorReason)error;
- (void)onProfileUpdate:(ChromeIdentity*)identity;
@end

// Simple observer bridge that forwards all events to its delegate observer.
class ChromeIdentityServiceObserverBridge
    : public ios::ChromeIdentityService::Observer {
 public:
  explicit ChromeIdentityServiceObserverBridge(
      id<ChromeIdentityServiceObserver> observer);
  ~ChromeIdentityServiceObserverBridge() override;

 private:
  // ios::ChromeIdentityService::Observer implementation.
  void OnIdentityListChanged() override;
  void OnAccessTokenRefreshFailed(ChromeIdentity* identity,
                                  ios::AccessTokenErrorReason error) override;
  void OnProfileUpdate(ChromeIdentity* identity) override;

  id<ChromeIdentityServiceObserver> observer_;  // Weak. |observer_| owns this.
  ScopedObserver<ios::ChromeIdentityService,
                 ChromeIdentityServiceObserverBridge> scoped_observer_;

  DISALLOW_COPY_AND_ASSIGN(ChromeIdentityServiceObserverBridge);
};

#endif  // IOS_CHROME_BROWSER_SIGNIN_CHROME_IDENTITY_SERVICE_OBSERVER_BRIDGE_H_
