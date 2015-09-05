// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/scoped_ptr.h"
#include "chrome/browser/apps/app_browsertest_util.h"
#include "chrome/browser/chromeos/app_mode/kiosk_app_manager.h"
#include "chrome/browser/chromeos/login/users/fake_chrome_user_manager.h"
#include "chrome/browser/chromeos/login/users/scoped_user_manager_enabler.h"
#include "chrome/browser/chromeos/ownership/fake_owner_settings_service.h"
#include "chrome/browser/chromeos/settings/scoped_cros_settings_test_helper.h"
#include "chrome/browser/extensions/extension_apitest.h"
#include "content/public/test/browser_test.h"
#include "extensions/common/switches.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace {
const std::string kTestingAppId = "pjdjhejcdkeebjehnokfbfnjmgmgdjlc";
}  // namespace

namespace extensions {
namespace {

// This class contains chrome.bluetoothLowEnergy API tests.
class BluetoothLowEnergyApiTestChromeOs : public PlatformAppBrowserTest {
 public:
  BluetoothLowEnergyApiTestChromeOs()
      : fake_user_manager_(nullptr), settings_helper_(false) {}
  ~BluetoothLowEnergyApiTestChromeOs() override {}

  void SetUpOnMainThread() override {
    PlatformAppBrowserTest::SetUpOnMainThread();
    settings_helper_.ReplaceProvider(
        chromeos::kAccountsPrefDeviceLocalAccounts);
    owner_settings_service_ =
        settings_helper_.CreateOwnerSettingsService(browser()->profile());
  }

  void TearDownOnMainThread() override {
    PlatformAppBrowserTest::TearDownOnMainThread();
    settings_helper_.RestoreProvider();
    user_manager_enabler_.reset();
    fake_user_manager_ = nullptr;
  }

 protected:
  chromeos::FakeChromeUserManager* fake_user_manager_;
  scoped_ptr<chromeos::ScopedUserManagerEnabler> user_manager_enabler_;

  chromeos::ScopedCrosSettingsTestHelper settings_helper_;
  scoped_ptr<chromeos::FakeOwnerSettingsService> owner_settings_service_;

  void EnterKioskSession() {
    fake_user_manager_ = new chromeos::FakeChromeUserManager();
    user_manager_enabler_.reset(
        new chromeos::ScopedUserManagerEnabler(fake_user_manager_));

    const std::string kKiosLogin = "kiosk@foobar.com";
    fake_user_manager_->AddKioskAppUser(kKiosLogin);
    fake_user_manager_->LoginUser(kKiosLogin);
  }

  void SetAutoLaunchApp() {
    manager()->AddApp(kTestingAppId, owner_settings_service_.get());
    manager()->SetAutoLaunchApp(kTestingAppId, owner_settings_service_.get());
    manager()->SetAppWasAutoLaunchedWithZeroDelay(kTestingAppId);
  }

  chromeos::KioskAppManager* manager() const {
    return chromeos::KioskAppManager::Get();
  }
};

IN_PROC_BROWSER_TEST_F(BluetoothLowEnergyApiTestChromeOs,
                       RegisterAdvertisement_Flag) {
  base::CommandLine::ForCurrentProcess()->AppendSwitch(
      switches::kEnableBLEAdvertising);
  ASSERT_TRUE(RunPlatformAppTest(
      "api_test/bluetooth_low_energy/register_advertisement_flag"))
      << message_;
}

IN_PROC_BROWSER_TEST_F(BluetoothLowEnergyApiTestChromeOs,
                       RegisterAdvertisement_NotKioskSession) {
  ASSERT_TRUE(RunPlatformAppTest(
      "api_test/bluetooth_low_energy/register_advertisement_no_kiosk_mode"))
      << message_;
}

IN_PROC_BROWSER_TEST_F(BluetoothLowEnergyApiTestChromeOs,
                       RegisterAdvertisement_KioskSessionOnly) {
  EnterKioskSession();
  ASSERT_TRUE(
      RunPlatformAppTest("api_test/bluetooth_low_energy/"
                         "register_advertisement_kiosk_session_only"))
      << message_;
}

IN_PROC_BROWSER_TEST_F(BluetoothLowEnergyApiTestChromeOs,
                       RegisterAdvertisement) {
  EnterKioskSession();
  SetAutoLaunchApp();
  ASSERT_TRUE(RunPlatformAppTest(
      "api_test/bluetooth_low_energy/register_advertisement"))
      << message_;
}

}  // namespace
}  // namespace extensions
