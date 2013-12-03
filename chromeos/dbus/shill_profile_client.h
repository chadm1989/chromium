// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_DBUS_SHILL_PROFILE_CLIENT_H_
#define CHROMEOS_DBUS_SHILL_PROFILE_CLIENT_H_

#include <string>

#include "base/basictypes.h"
#include "base/callback.h"
#include "chromeos/chromeos_export.h"
#include "chromeos/dbus/dbus_client.h"
#include "chromeos/dbus/shill_client_helper.h"

namespace base {

class Value;
class DictionaryValue;

}  // namespace base

namespace dbus {

class ObjectPath;

}  // namespace dbus

namespace chromeos {

class ShillPropertyChangedObserver;

// ShillProfileClient is used to communicate with the Shill Profile
// service.  All methods should be called from the origin thread which
// initializes the DBusThreadManager instance.
class CHROMEOS_EXPORT ShillProfileClient : public DBusClient {
 public:
  typedef ShillClientHelper::PropertyChangedHandler PropertyChangedHandler;
  typedef ShillClientHelper::DictionaryValueCallbackWithoutStatus
      DictionaryValueCallbackWithoutStatus;
  typedef ShillClientHelper::ErrorCallback ErrorCallback;

  // Interface for setting up services for testing. Accessed through
  // GetTestInterface(), only implemented in the stub implementation.
  // TODO(stevenjb): remove dependencies on entry_path -> service_path
  // mappings in some of the TestInterface implementations.
  class TestInterface {
   public:
    virtual void AddProfile(const std::string& profile_path,
                            const std::string& userhash) = 0;

    // Adds an entry to the profile only. |entry_path| corresponds to a
    // 'service_path' and a corresponding entry will be added to
    // ShillManagerClient ServiceCompleteList. No checking or updating of
    // ShillServiceClient is performed.
    virtual void AddEntry(const std::string& profile_path,
                          const std::string& entry_path,
                          const base::DictionaryValue& properties) = 0;

    // Adds a service to the profile, copying properties from the
    // ShillServiceClient entry (which must be present). Also sets the Profile
    // property of the service in ShillServiceClient.
    virtual bool AddService(const std::string& profile_path,
                            const std::string& service_path) = 0;

    // Sets |profiles| to the current list of profile paths.
    virtual void GetProfilePaths(std::vector<std::string>* profiles) = 0;

   protected:
    virtual ~TestInterface() {}
  };

  virtual ~ShillProfileClient();

  // Factory function, creates a new instance which is owned by the caller.
  // For normal usage, access the singleton via DBusThreadManager::Get().
  static ShillProfileClient* Create();

  // Adds a property changed |observer| for the profile at |profile_path|.
  virtual void AddPropertyChangedObserver(
      const dbus::ObjectPath& profile_path,
      ShillPropertyChangedObserver* observer) = 0;

  // Removes a property changed |observer| for the profile at |profile_path|.
  virtual void RemovePropertyChangedObserver(
      const dbus::ObjectPath& profile_path,
      ShillPropertyChangedObserver* observer) = 0;

  // Calls GetProperties method.
  // |callback| is called after the method call succeeds.
  virtual void GetProperties(
      const dbus::ObjectPath& profile_path,
      const DictionaryValueCallbackWithoutStatus& callback,
      const ErrorCallback& error_callback) = 0;

  // Calls GetEntry method.
  // |callback| is called after the method call succeeds.
  virtual void GetEntry(const dbus::ObjectPath& profile_path,
                        const std::string& entry_path,
                        const DictionaryValueCallbackWithoutStatus& callback,
                        const ErrorCallback& error_callback) = 0;

  // Calls DeleteEntry method.
  // |callback| is called after the method call succeeds.
  virtual void DeleteEntry(const dbus::ObjectPath& profile_path,
                           const std::string& entry_path,
                           const base::Closure& callback,
                           const ErrorCallback& error_callback) = 0;

  // Returns an interface for testing (stub only), or returns NULL.
  virtual TestInterface* GetTestInterface() = 0;

 protected:
  friend class ShillProfileClientTest;

  // Create() should be used instead.
  ShillProfileClient();

 private:
  DISALLOW_COPY_AND_ASSIGN(ShillProfileClient);
};

}  // namespace chromeos

#endif  // CHROMEOS_DBUS_SHILL_PROFILE_CLIENT_H_
