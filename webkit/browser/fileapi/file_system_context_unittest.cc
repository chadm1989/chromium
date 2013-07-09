// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webkit/browser/fileapi/file_system_context.h"

#include "base/files/scoped_temp_dir.h"
#include "base/message_loop.h"
#include "base/strings/stringprintf.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "webkit/browser/fileapi/external_mount_points.h"
#include "webkit/browser/fileapi/file_system_backend.h"
#include "webkit/browser/fileapi/file_system_task_runners.h"
#include "webkit/browser/fileapi/isolated_context.h"
#include "webkit/browser/fileapi/mock_file_system_options.h"
#include "webkit/browser/quota/mock_quota_manager.h"
#include "webkit/browser/quota/mock_special_storage_policy.h"

#define FPL(x) FILE_PATH_LITERAL(x)

#if defined(FILE_PATH_USES_DRIVE_LETTERS)
#define DRIVE FPL("C:")
#else
#define DRIVE
#endif

namespace fileapi {

namespace {

const char kTestOrigin[] = "http://chromium.org/";
const base::FilePath::CharType kVirtualPathNoRoot[] = FPL("root/file");

GURL CreateRawFileSystemURL(const std::string& type_str,
                            const std::string& fs_id) {
  std::string url_str = base::StringPrintf(
      "filesystem:http://chromium.org/%s/%s/root/file",
      type_str.c_str(),
      fs_id.c_str());
  return GURL(url_str);
}

class FileSystemContextTest : public testing::Test {
 public:
  FileSystemContextTest() {}

  virtual void SetUp() {
    ASSERT_TRUE(data_dir_.CreateUniqueTempDir());

    storage_policy_ = new quota::MockSpecialStoragePolicy();

    mock_quota_manager_ =
        new quota::MockQuotaManager(false /* is_incognito */,
                                    data_dir_.path(),
                                    base::MessageLoopProxy::current().get(),
                                    base::MessageLoopProxy::current().get(),
                                    storage_policy_.get());
  }

 protected:
  FileSystemContext* CreateFileSystemContextForTest(
      ExternalMountPoints* external_mount_points) {
    return new FileSystemContext(FileSystemTaskRunners::CreateMockTaskRunners(),
                                 external_mount_points,
                                 storage_policy_.get(),
                                 mock_quota_manager_->proxy(),
                                 ScopedVector<FileSystemBackend>(),
                                 data_dir_.path(),
                                 CreateAllowFileAccessOptions());
  }

  // Verifies a *valid* filesystem url has expected values.
  void ExpectFileSystemURLMatches(const FileSystemURL& url,
                                  const GURL& expect_origin,
                                  FileSystemType expect_mount_type,
                                  FileSystemType expect_type,
                                  const base::FilePath& expect_path,
                                  const base::FilePath& expect_virtual_path,
                                  const std::string& expect_filesystem_id) {
    EXPECT_TRUE(url.is_valid());

    EXPECT_EQ(expect_origin, url.origin());
    EXPECT_EQ(expect_mount_type, url.mount_type());
    EXPECT_EQ(expect_type, url.type());
    EXPECT_EQ(expect_path, url.path());
    EXPECT_EQ(expect_virtual_path, url.virtual_path());
    EXPECT_EQ(expect_filesystem_id, url.filesystem_id());
  }

 private:
  base::ScopedTempDir data_dir_;
  base::MessageLoop message_loop_;
  scoped_refptr<quota::SpecialStoragePolicy> storage_policy_;
  scoped_refptr<quota::MockQuotaManager> mock_quota_manager_;
};

// It is not valid to pass NULL ExternalMountPoints to FileSystemContext on
// ChromeOS.
#if !defined(OS_CHROMEOS)
TEST_F(FileSystemContextTest, NullExternalMountPoints) {
  scoped_refptr<FileSystemContext> file_system_context(
      CreateFileSystemContextForTest(NULL));

  // Cracking system external mount and isolated mount points should work.
  std::string isolated_name = "root";
  std::string isolated_id =
      IsolatedContext::GetInstance()->RegisterFileSystemForPath(
          kFileSystemTypeNativeLocal,
          base::FilePath(DRIVE FPL("/test/isolated/root")),
          &isolated_name);
  // Register system external mount point.
  ASSERT_TRUE(ExternalMountPoints::GetSystemInstance()->RegisterFileSystem(
      "system",
      kFileSystemTypeNativeLocal,
      base::FilePath(DRIVE FPL("/test/sys/"))));

  FileSystemURL cracked_isolated = file_system_context->CrackURL(
      CreateRawFileSystemURL("isolated", isolated_id));

  ExpectFileSystemURLMatches(
      cracked_isolated,
      GURL(kTestOrigin),
      kFileSystemTypeIsolated,
      kFileSystemTypeNativeLocal,
      base::FilePath(DRIVE FPL("/test/isolated/root/file")).NormalizePathSeparators(),
      base::FilePath::FromUTF8Unsafe(isolated_id).Append(FPL("root/file")).
          NormalizePathSeparators(),
      isolated_id);

  FileSystemURL cracked_external = file_system_context->CrackURL(
      CreateRawFileSystemURL("external", "system"));

  ExpectFileSystemURLMatches(
      cracked_external,
      GURL(kTestOrigin),
      kFileSystemTypeExternal,
      kFileSystemTypeNativeLocal,
      base::FilePath(
          DRIVE FPL("/test/sys/root/file")).NormalizePathSeparators(),
      base::FilePath(FPL("system/root/file")).NormalizePathSeparators(),
      "system");


  IsolatedContext::GetInstance()->RevokeFileSystem(isolated_id);
  ExternalMountPoints::GetSystemInstance()->RevokeFileSystem("system");
}
#endif  // !defiend(OS_CHROMEOS)

TEST_F(FileSystemContextTest, FileSystemContextKeepsMountPointsAlive) {
  scoped_refptr<ExternalMountPoints> mount_points =
      ExternalMountPoints::CreateRefCounted();

  // Register system external mount point.
  ASSERT_TRUE(mount_points->RegisterFileSystem(
      "system",
      kFileSystemTypeNativeLocal,
      base::FilePath(DRIVE FPL("/test/sys/"))));

  scoped_refptr<FileSystemContext> file_system_context(
      CreateFileSystemContextForTest(mount_points.get()));

  // Release a MountPoints reference created in the test.
  mount_points = NULL;

  // FileSystemContext should keep a reference to the |mount_points|, so it
  // should be able to resolve the URL.
  FileSystemURL cracked_external = file_system_context->CrackURL(
      CreateRawFileSystemURL("external", "system"));

  ExpectFileSystemURLMatches(
      cracked_external,
      GURL(kTestOrigin),
      kFileSystemTypeExternal,
      kFileSystemTypeNativeLocal,
      base::FilePath(
          DRIVE FPL("/test/sys/root/file")).NormalizePathSeparators(),
      base::FilePath(FPL("system/root/file")).NormalizePathSeparators(),
      "system");

  // No need to revoke the registered filesystem since |mount_points| lifetime
  // is bound to this test.
}

TEST_F(FileSystemContextTest, CrackFileSystemURL) {
  scoped_refptr<ExternalMountPoints> external_mount_points(
      ExternalMountPoints::CreateRefCounted());
  scoped_refptr<FileSystemContext> file_system_context(
      CreateFileSystemContextForTest(external_mount_points.get()));

  // Register an isolated mount point.
  std::string isolated_file_system_name = "root";
  const std::string kIsolatedFileSystemID =
      IsolatedContext::GetInstance()->RegisterFileSystemForPath(
          kFileSystemTypeNativeLocal,
          base::FilePath(DRIVE FPL("/test/isolated/root")),
          &isolated_file_system_name);
  // Register system external mount point.
  ASSERT_TRUE(ExternalMountPoints::GetSystemInstance()->RegisterFileSystem(
      "system",
      kFileSystemTypeDrive,
      base::FilePath(DRIVE FPL("/test/sys/"))));
  ASSERT_TRUE(ExternalMountPoints::GetSystemInstance()->RegisterFileSystem(
      "ext",
      kFileSystemTypeNativeLocal,
      base::FilePath(DRIVE FPL("/test/ext"))));
  // Register a system external mount point with the same name/id as the
  // registered isolated mount point.
  ASSERT_TRUE(ExternalMountPoints::GetSystemInstance()->RegisterFileSystem(
      kIsolatedFileSystemID,
      kFileSystemTypeRestrictedNativeLocal,
      base::FilePath(DRIVE FPL("/test/system/isolated"))));
  // Add a mount points with the same name as a system mount point to
  // FileSystemContext's external mount points.
  ASSERT_TRUE(external_mount_points->RegisterFileSystem(
      "ext",
       kFileSystemTypeNativeLocal,
       base::FilePath(DRIVE FPL("/test/local/ext/"))));

  const GURL kTestOrigin = GURL("http://chromium.org/");
  const base::FilePath kVirtualPathNoRoot = base::FilePath(FPL("root/file"));

  struct TestCase {
    // Test case values.
    std::string root;
    std::string type_str;

    // Expected test results.
    bool expect_is_valid;
    FileSystemType expect_mount_type;
    FileSystemType expect_type;
    const base::FilePath::CharType* expect_path;
    std::string expect_filesystem_id;
  };

  const TestCase kTestCases[] = {
      // Following should not be handled by the url crackers:
      {
        "pers_mount", "persistent", true /* is_valid */,
        kFileSystemTypePersistent, kFileSystemTypePersistent,
        FPL("pers_mount/root/file"),
        std::string()  /* filesystem id */
      },
      {
        "temp_mount", "temporary", true /* is_valid */,
        kFileSystemTypeTemporary, kFileSystemTypeTemporary,
        FPL("temp_mount/root/file"),
        std::string()  /* filesystem id */
      },
      // Should be cracked by isolated mount points:
      {
        kIsolatedFileSystemID, "isolated", true /* is_valid */,
        kFileSystemTypeIsolated, kFileSystemTypeNativeLocal,
        DRIVE FPL("/test/isolated/root/file"),
        kIsolatedFileSystemID
      },
      // Should be cracked by system mount points:
      {
        "system", "external", true /* is_valid */,
        kFileSystemTypeExternal, kFileSystemTypeDrive,
        DRIVE FPL("/test/sys/root/file"),
        "system"
      },
      {
        kIsolatedFileSystemID, "external", true /* is_valid */,
        kFileSystemTypeExternal, kFileSystemTypeRestrictedNativeLocal,
        DRIVE FPL("/test/system/isolated/root/file"),
        kIsolatedFileSystemID
      },
      // Should be cracked by FileSystemContext's ExternalMountPoints.
      {
        "ext", "external", true /* is_valid */,
        kFileSystemTypeExternal, kFileSystemTypeNativeLocal,
        DRIVE FPL("/test/local/ext/root/file"),
        "ext"
      },
      // Test for invalid filesystem url (made invalid by adding invalid
      // filesystem type).
      {
        "sytem", "external", false /* is_valid */,
        // The rest of values will be ignored.
        kFileSystemTypeUnknown, kFileSystemTypeUnknown, FPL(""),
        std::string()
      },
      // Test for URL with non-existing filesystem id.
      {
        "invalid", "external", false /* is_valid */,
        // The rest of values will be ignored.
        kFileSystemTypeUnknown, kFileSystemTypeUnknown, FPL(""),
        std::string()
      },
  };

  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(kTestCases); ++i) {
    const base::FilePath virtual_path =
        base::FilePath::FromUTF8Unsafe(kTestCases[i].root).Append(kVirtualPathNoRoot);

    GURL raw_url =
        CreateRawFileSystemURL(kTestCases[i].type_str, kTestCases[i].root);
    FileSystemURL cracked_url = file_system_context->CrackURL(raw_url);

    SCOPED_TRACE(testing::Message() << "Test case " << i << ": "
                                    << "Cracking URL: " << raw_url);

    EXPECT_EQ(kTestCases[i].expect_is_valid, cracked_url.is_valid());
    if (!kTestCases[i].expect_is_valid)
      continue;

    ExpectFileSystemURLMatches(
        cracked_url,
        GURL(kTestOrigin),
        kTestCases[i].expect_mount_type,
        kTestCases[i].expect_type,
        base::FilePath(kTestCases[i].expect_path).NormalizePathSeparators(),
        virtual_path.NormalizePathSeparators(),
        kTestCases[i].expect_filesystem_id);
  }

  IsolatedContext::GetInstance()->RevokeFileSystemByPath(
      base::FilePath(DRIVE FPL("/test/isolated/root")));
  ExternalMountPoints::GetSystemInstance()->RevokeFileSystem("system");
  ExternalMountPoints::GetSystemInstance()->RevokeFileSystem("ext");
  ExternalMountPoints::GetSystemInstance()->RevokeFileSystem(
      kIsolatedFileSystemID);
}

}  // namespace

}  // namespace fileapi
