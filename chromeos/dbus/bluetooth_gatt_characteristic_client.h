// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_DBUS_BLUETOOTH_GATT_CHARACTERISTIC_CLIENT_H_
#define CHROMEOS_DBUS_BLUETOOTH_GATT_CHARACTERISTIC_CLIENT_H_

#include <string>

#include "base/basictypes.h"
#include "base/callback.h"
#include "chromeos/chromeos_export.h"
#include "chromeos/dbus/dbus_client.h"
#include "dbus/object_path.h"
#include "dbus/property.h"

namespace chromeos {

// BluetoothGattCharacteristicClient is used to communicate with remote GATT
// characteristic objects exposed by the Bluetooth daemon.
class CHROMEOS_EXPORT BluetoothGattCharacteristicClient : public DBusClient {
 public:
  // Structure of properties associated with GATT characteristics.
  struct Properties : public dbus::PropertySet {
    // The 128-bit characteristic UUID. [read-only]
    dbus::Property<std::string> uuid;

    // Object path of the GATT service the characteristic belongs to.
    // [read-only]
    dbus::Property<dbus::ObjectPath> service;

    // Whether or not this characteristic is currently sending ValueUpdated
    // signals.
    dbus::Property<bool> notifying;

    // List of flags representing the GATT "Characteristic Properties bit field"
    // and properties read from the GATT "Characteristic Extended Properties"
    // descriptor bit field. [read-only, optional]
    dbus::Property<std::vector<std::string> > flags;

    Properties(dbus::ObjectProxy* object_proxy,
               const std::string& interface_name,
               const PropertyChangedCallback& callback);
    virtual ~Properties();
  };

  // Interface for observing changes from a remote GATT characteristic.
  class Observer {
   public:
    virtual ~Observer() {}

    // Called when the GATT characteristic with object path |object_path| is
    // added to the system.
    virtual void GattCharacteristicAdded(const dbus::ObjectPath& object_path) {}

    // Called when the GATT characteristic with object path |object_path| is
    // removed from the system.
    virtual void GattCharacteristicRemoved(
        const dbus::ObjectPath& object_path) {}

    // Called when the GATT characteristic with object path |object_path| has a
    // change in the value of the property named |property_name|.
    virtual void GattCharacteristicPropertyChanged(
        const dbus::ObjectPath& object_path,
        const std::string& property_name) {}

    // Called when a "ValueUpdated" signal is received from the remote GATT
    // characteristic with object path |object_path| with characteristic value
    // |value|.
    virtual void GattCharacteristicValueUpdated(
        const dbus::ObjectPath& object_path,
        const std::vector<uint8>& value) {}
  };

  // Callbacks used to report the result of asynchronous methods.
  typedef base::Callback<void(const std::string& error_name,
                              const std::string& error_message)> ErrorCallback;
  typedef base::Callback<void(const std::vector<uint8>& value)> ValueCallback;

  virtual ~BluetoothGattCharacteristicClient();

  // Adds and removes observers for events on all remote GATT characteristics.
  // Check the |object_path| parameter of observer methods to determine which
  // GATT characteristic is issuing the event.
  virtual void AddObserver(Observer* observer) = 0;
  virtual void RemoveObserver(Observer* observer) = 0;

  // Returns the list of GATT characteristic object paths known to the system.
  virtual std::vector<dbus::ObjectPath> GetCharacteristics() = 0;

  // Obtain the properties for the GATT characteristic with object path
  // |object_path|. Values should be copied if needed.
  virtual Properties* GetProperties(const dbus::ObjectPath& object_path) = 0;

  // Issues a request to read the value of GATT characteristic with object path
  // |object_path| and returns the value in |callback| on success. On error,
  // invokes |error_callback|.
  virtual void ReadValue(const dbus::ObjectPath& object_path,
                         const ValueCallback& callback,
                         const ErrorCallback& error_callback) = 0;

  // Issues a request to write the value of GATT characteristic with object path
  // |object_path| with value |value|. Invokes |callback| on success and
  // |error_callback| on failure.
  virtual void WriteValue(const dbus::ObjectPath& object_path,
                          const std::vector<uint8>& value,
                          const base::Closure& callback,
                          const ErrorCallback& error_callback) = 0;

  // Starts a notification session from this characteristic with object path
  // |object_path| if it supports value notifications or indications. Invokes
  // |callback| on success and |error_callback| on failure.
  virtual void StartNotify(const dbus::ObjectPath& object_path,
                           const base::Closure& callback,
                           const ErrorCallback& error_callback) = 0;

  // Cancels any previous StartNotify transaction for characteristic with
  // object path |object_path|. Invokes |callback| on success and
  // |error_callback| on failure.
  virtual void StopNotify(const dbus::ObjectPath& object_path,
                          const base::Closure& callback,
                          const ErrorCallback& error_callback) = 0;

  // Creates the instance.
  static BluetoothGattCharacteristicClient* Create();

  // Constants used to indicate exceptional error conditions.
  static const char kNoResponseError[];
  static const char kUnknownCharacteristicError[];

 protected:
  BluetoothGattCharacteristicClient();

 private:
  DISALLOW_COPY_AND_ASSIGN(BluetoothGattCharacteristicClient);
};

}  // namespace chromeos

#endif  // CHROMEOS_DBUS_BLUETOOTH_GATT_CHARACTERISTIC_CLIENT_H_
