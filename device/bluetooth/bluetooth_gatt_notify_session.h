// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_BLUETOOTH_BLUETOOTH_GATT_NOTIFY_SESSION_H_
#define DEVICE_BLUETOOTH_BLUETOOTH_GATT_NOTIFY_SESSION_H_

#include <string>

#include "base/callback.h"

namespace device {

// A BluetoothGattNotifySession represents an active session for listening
// to value updates from GATT characteristics that support notifications and/or
// indications. Instances are obtained by calling
// BluetoothGattCharacteristic::StartNotifySession.
class BluetoothGattNotifySession {
 public:
  // Destructor autmatically stops this session.
  virtual ~BluetoothGattNotifySession();

  // Returns the identifier of the associated characteristic.
  virtual std::string GetCharacteristicIdentifier() const = 0;

  // Returns true if this session is active.
  virtual bool IsActive() = 0;

  // Stops this session and calls |callback| upon completion. This won't
  // necessarily stop value updates from the characteristic -- since updates
  // are shared among BluetoothGattNotifySession instances -- but it will
  // terminate this session.
  virtual void Stop(const base::Closure& callback) = 0;

 protected:
  BluetoothGattNotifySession();

 private:
  DISALLOW_COPY_AND_ASSIGN(BluetoothGattNotifySession);
};

}  // namespace device

#endif  // DEVICE_BLUETOOTH_BLUETOOTH_GATT_NOTIFY_SESSION_H_
