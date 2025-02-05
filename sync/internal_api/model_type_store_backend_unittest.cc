// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sync/internal_api/public/model_type_store_backend.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/leveldatabase/src/include/leveldb/env.h"
#include "third_party/leveldatabase/src/include/leveldb/write_batch.h"

namespace syncer_v2 {

class ModelTypeStoreBackendTest : public testing::Test {
 public:
  scoped_refptr<ModelTypeStoreBackend> GetOrCreateBackend() {
    std::string path = "/test_db";
    return GetOrCreateBackendWithPath(path);
  }

  scoped_refptr<ModelTypeStoreBackend> GetOrCreateBackendWithPath(
      std::string custom_path) {
    std::unique_ptr<leveldb::Env> in_memory_env =
        ModelTypeStoreBackend::CreateInMemoryEnv();
    std::string path;
    in_memory_env->GetTestDirectory(&path);
    path += custom_path;

    ModelTypeStore::Result result;
    // In-memory store backend works on the same thread as test.
    scoped_refptr<ModelTypeStoreBackend> backend =
        ModelTypeStoreBackend::GetOrCreateBackend(
            path, std::move(in_memory_env), &result);
    EXPECT_TRUE(backend.get());
    EXPECT_EQ(result, ModelTypeStore::Result::SUCCESS);
    return backend;
  }

  bool BackendExistsForPath(std::string path) {
    if (ModelTypeStoreBackend::backend_map_.Get().end() ==
        ModelTypeStoreBackend::backend_map_.Get().find(path)) {
      return false;
    }
    return true;
  }

  std::string GetBackendPath(scoped_refptr<ModelTypeStoreBackend> backend) {
    return backend->path_;
  }
};

// Test that after record is written to backend it can be read back even after
// backend is destroyed and recreated in the same environment.
TEST_F(ModelTypeStoreBackendTest, WriteThenRead) {
  scoped_refptr<ModelTypeStoreBackend> backend = GetOrCreateBackend();

  // Write record.
  std::unique_ptr<leveldb::WriteBatch> write_batch(new leveldb::WriteBatch());
  write_batch->Put("prefix:id1", "data1");
  ModelTypeStore::Result result =
      backend->WriteModifications(std::move(write_batch));
  ASSERT_EQ(ModelTypeStore::Result::SUCCESS, result);

  // Read all records with prefix.
  ModelTypeStore::RecordList record_list;
  result = backend->ReadAllRecordsWithPrefix("prefix:", &record_list);
  ASSERT_EQ(ModelTypeStore::Result::SUCCESS, result);
  ASSERT_EQ(1ul, record_list.size());
  ASSERT_EQ("id1", record_list[0].id);
  ASSERT_EQ("data1", record_list[0].value);
  record_list.clear();

  // Recreate backend and read all records with prefix.
  backend = GetOrCreateBackend();
  result = backend->ReadAllRecordsWithPrefix("prefix:", &record_list);
  ASSERT_EQ(ModelTypeStore::Result::SUCCESS, result);
  ASSERT_EQ(1ul, record_list.size());
  ASSERT_EQ("id1", record_list[0].id);
  ASSERT_EQ("data1", record_list[0].value);
}

// Test that ReadAllRecordsWithPrefix correclty filters records by prefix.
TEST_F(ModelTypeStoreBackendTest, ReadAllRecordsWithPrefix) {
  scoped_refptr<ModelTypeStoreBackend> backend = GetOrCreateBackend();

  std::unique_ptr<leveldb::WriteBatch> write_batch(new leveldb::WriteBatch());
  write_batch->Put("prefix1:id1", "data1");
  write_batch->Put("prefix2:id2", "data2");
  ModelTypeStore::Result result =
      backend->WriteModifications(std::move(write_batch));
  ASSERT_EQ(ModelTypeStore::Result::SUCCESS, result);

  ModelTypeStore::RecordList record_list;
  result = backend->ReadAllRecordsWithPrefix("prefix1:", &record_list);
  ASSERT_EQ(ModelTypeStore::Result::SUCCESS, result);
  ASSERT_EQ(1UL, record_list.size());
  ASSERT_EQ("id1", record_list[0].id);
  ASSERT_EQ("data1", record_list[0].value);
}

// Test that deleted records are correctly marked as milling in results of
// ReadRecordsWithPrefix.
TEST_F(ModelTypeStoreBackendTest, ReadDeletedRecord) {
  scoped_refptr<ModelTypeStoreBackend> backend = GetOrCreateBackend();

  // Create records, ensure they are returned by ReadRecordsWithPrefix.
  std::unique_ptr<leveldb::WriteBatch> write_batch(new leveldb::WriteBatch());
  write_batch->Put("prefix:id1", "data1");
  write_batch->Put("prefix:id2", "data2");
  ModelTypeStore::Result result =
      backend->WriteModifications(std::move(write_batch));
  ASSERT_EQ(ModelTypeStore::Result::SUCCESS, result);

  ModelTypeStore::IdList id_list;
  ModelTypeStore::IdList missing_id_list;
  ModelTypeStore::RecordList record_list;
  id_list.push_back("id1");
  id_list.push_back("id2");
  result = backend->ReadRecordsWithPrefix("prefix:", id_list, &record_list,
                                          &missing_id_list);
  ASSERT_EQ(ModelTypeStore::Result::SUCCESS, result);
  ASSERT_EQ(2UL, record_list.size());
  ASSERT_TRUE(missing_id_list.empty());

  // Delete one record.
  write_batch.reset(new leveldb::WriteBatch());
  write_batch->Delete("prefix:id2");
  result = backend->WriteModifications(std::move(write_batch));
  ASSERT_EQ(ModelTypeStore::Result::SUCCESS, result);

  // Ensure deleted record id is returned in missing_id_list.
  record_list.clear();
  missing_id_list.clear();
  result = backend->ReadRecordsWithPrefix("prefix:", id_list, &record_list,
                                          &missing_id_list);
  ASSERT_EQ(ModelTypeStore::Result::SUCCESS, result);
  ASSERT_EQ(1UL, record_list.size());
  ASSERT_EQ("id1", record_list[0].id);
  ASSERT_EQ(1UL, missing_id_list.size());
  ASSERT_EQ("id2", missing_id_list[0]);
}

// Test that only one backend got create when we ask two backend with same path,
// and after de-reference the backend, the backend will be deleted.
TEST_F(ModelTypeStoreBackendTest, TwoSameBackendTest) {
  // Create two backend with same path, check if they are reference to same
  // address.
  scoped_refptr<ModelTypeStoreBackend> backend = GetOrCreateBackend();
  scoped_refptr<ModelTypeStoreBackend> backend_second = GetOrCreateBackend();
  std::string path = GetBackendPath(backend);
  ASSERT_EQ(backend.get(), backend_second.get());

  // Delete one reference, check the real backend still here.
  backend = nullptr;
  ASSERT_FALSE(backend.get());
  ASSERT_TRUE(backend_second.get());
  ASSERT_TRUE(backend_second->HasOneRef());

  // Delete another reference, check the real backend is deleted.
  backend_second = nullptr;
  ASSERT_FALSE(backend_second.get());
  ASSERT_FALSE(BackendExistsForPath(path));
}

// Test that two backend got create when we ask two backend with different path,
// and after de-reference two backend, the both backend will be deleted.
TEST_F(ModelTypeStoreBackendTest, TwoDifferentBackendTest) {
  // Create two backend with different path, check if they are reference to
  // different address.
  scoped_refptr<ModelTypeStoreBackend> backend = GetOrCreateBackend();
  scoped_refptr<ModelTypeStoreBackend> backend_second =
      GetOrCreateBackendWithPath("/test_db2");
  std::string path = GetBackendPath(backend);
  ASSERT_NE(backend.get(), backend_second.get());
  ASSERT_TRUE(backend->HasOneRef());
  ASSERT_TRUE(backend_second->HasOneRef());

  // delete one backend, check only one got deleted.
  backend = nullptr;
  ASSERT_FALSE(backend.get());
  ASSERT_TRUE(backend_second.get());
  ASSERT_TRUE(backend_second->HasOneRef());
  ASSERT_FALSE(BackendExistsForPath(path));

  // delete another backend.
  backend_second = nullptr;
  ASSERT_FALSE(backend_second.get());
  ASSERT_FALSE(BackendExistsForPath("/test_db2"));
}

}  // namespace syncer_v2
