// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "base/files/file.h"
#include "base/memory/scoped_ptr.h"
#include "chrome/browser/chromeos/file_system_provider/fake_provided_file_system.h"
#include "chrome/browser/chromeos/file_system_provider/mount_path_util.h"
#include "chrome/browser/chromeos/file_system_provider/provided_file_system_interface.h"
#include "chrome/browser/chromeos/file_system_provider/service.h"
#include "chrome/browser/chromeos/file_system_provider/service_factory.h"
#include "chrome/browser/chromeos/login/users/fake_user_manager.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "components/keyed_service/core/keyed_service.h"
#include "content/public/browser/browser_context.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "extensions/browser/extension_registry.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "webkit/browser/fileapi/external_mount_points.h"

namespace chromeos {
namespace file_system_provider {
namespace util {

namespace {

const char kExtensionId[] = "mbflcebpggnecokmikipoihdbecnjfoj";
const char kFileSystemName[] = "Camera Pictures";

// Creates a FileSystemURL for tests.
fileapi::FileSystemURL CreateFileSystemURL(Profile* profile,
                                           const std::string& extension_id,
                                           int file_system_id,
                                           const base::FilePath& file_path) {
  const std::string origin = std::string("chrome-extension://") + kExtensionId;
  const base::FilePath mount_path =
      util::GetMountPath(profile, extension_id, file_system_id);
  const fileapi::ExternalMountPoints* const mount_points =
      fileapi::ExternalMountPoints::GetSystemInstance();
  DCHECK(mount_points);
  DCHECK(file_path.IsAbsolute());
  base::FilePath relative_path(file_path.value().substr(1));
  return mount_points->CreateCrackedFileSystemURL(
      GURL(origin),
      fileapi::kFileSystemTypeExternal,
      base::FilePath(mount_path.BaseName().Append(relative_path)));
}

// Creates a Service instance. Used to be able to destroy the service in
// TearDown().
KeyedService* CreateService(content::BrowserContext* context) {
  return new Service(Profile::FromBrowserContext(context),
                     extensions::ExtensionRegistry::Get(context));
}

}  // namespace

class FileSystemProviderMountPathUtilTest : public testing::Test {
 protected:
  FileSystemProviderMountPathUtilTest() {}
  virtual ~FileSystemProviderMountPathUtilTest() {}

  virtual void SetUp() OVERRIDE {
    profile_manager_.reset(
        new TestingProfileManager(TestingBrowserProcess::GetGlobal()));
    ASSERT_TRUE(profile_manager_->SetUp());
    profile_ = profile_manager_->CreateTestingProfile("testing-profile");
    user_manager_ = new FakeUserManager();
    user_manager_enabler_.reset(new ScopedUserManagerEnabler(user_manager_));
    user_manager_->AddUser(profile_->GetProfileName());
    ServiceFactory::GetInstance()->SetTestingFactory(profile_, &CreateService);
    file_system_provider_service_ = Service::Get(profile_);
    file_system_provider_service_->SetFileSystemFactoryForTests(
        base::Bind(&FakeProvidedFileSystem::Create));
  }

  virtual void TearDown() OVERRIDE {
    // Setting the testing factory to NULL will destroy the created service
    // associated with the testing profile.
    ServiceFactory::GetInstance()->SetTestingFactory(profile_, NULL);
  }

  content::TestBrowserThreadBundle thread_bundle_;
  scoped_ptr<TestingProfileManager> profile_manager_;
  TestingProfile* profile_;  // Owned by TestingProfileManager.
  scoped_ptr<ScopedUserManagerEnabler> user_manager_enabler_;
  FakeUserManager* user_manager_;
  Service* file_system_provider_service_;  // Owned by its factory.
};

TEST_F(FileSystemProviderMountPathUtilTest, GetMountPath) {
  const std::string kExtensionId = "mbflcebpggnecokmikipoihdbecnjfoj";
  const int kFileSystemId = 1;

  base::FilePath result = GetMountPath(profile_, kExtensionId, kFileSystemId);
  EXPECT_EQ("/provided/mbflcebpggnecokmikipoihdbecnjfoj-1-testing-profile-hash",
            result.AsUTF8Unsafe());
}

TEST_F(FileSystemProviderMountPathUtilTest, Parser) {
  const int file_system_id = file_system_provider_service_->MountFileSystem(
      kExtensionId, kFileSystemName);
  EXPECT_LT(0, file_system_id);

  const base::FilePath kFilePath =
      base::FilePath::FromUTF8Unsafe("/hello/world.txt");
  const fileapi::FileSystemURL url =
      CreateFileSystemURL(profile_, kExtensionId, file_system_id, kFilePath);
  EXPECT_TRUE(url.is_valid());

  FileSystemURLParser parser(url);
  EXPECT_TRUE(parser.Parse());

  ProvidedFileSystemInterface* file_system = parser.file_system();
  ASSERT_TRUE(file_system);
  EXPECT_EQ(file_system_id, file_system->GetFileSystemInfo().file_system_id());
  EXPECT_EQ(kFilePath.AsUTF8Unsafe(), parser.file_path().AsUTF8Unsafe());
}

TEST_F(FileSystemProviderMountPathUtilTest, Parser_RootPath) {
  const int file_system_id = file_system_provider_service_->MountFileSystem(
      kExtensionId, kFileSystemName);
  EXPECT_LT(0, file_system_id);

  const base::FilePath kFilePath = base::FilePath::FromUTF8Unsafe("/");
  const fileapi::FileSystemURL url =
      CreateFileSystemURL(profile_, kExtensionId, file_system_id, kFilePath);
  EXPECT_TRUE(url.is_valid());

  FileSystemURLParser parser(url);
  EXPECT_TRUE(parser.Parse());

  ProvidedFileSystemInterface* file_system = parser.file_system();
  ASSERT_TRUE(file_system);
  EXPECT_EQ(file_system_id, file_system->GetFileSystemInfo().file_system_id());
  EXPECT_EQ(kFilePath.AsUTF8Unsafe(), parser.file_path().AsUTF8Unsafe());
}

TEST_F(FileSystemProviderMountPathUtilTest, Parser_WrongUrl) {
  const int file_system_id = file_system_provider_service_->MountFileSystem(
      kExtensionId, kFileSystemName);
  EXPECT_LT(0, file_system_id);

  const base::FilePath kFilePath = base::FilePath::FromUTF8Unsafe("/hello");
  const fileapi::FileSystemURL url = CreateFileSystemURL(
      profile_, kExtensionId, file_system_id + 1, kFilePath);
  // It is impossible to create a cracked URL for a mount point which doesn't
  // exist, therefore is will always be invalid, and empty.
  EXPECT_FALSE(url.is_valid());

  FileSystemURLParser parser(url);
  EXPECT_FALSE(parser.Parse());
}

}  // namespace util
}  // namespace file_system_provider
}  // namespace chromeos
