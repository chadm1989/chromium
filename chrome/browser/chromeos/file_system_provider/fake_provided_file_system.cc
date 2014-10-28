// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/file_system_provider/fake_provided_file_system.h"

#include "base/files/file.h"
#include "base/message_loop/message_loop_proxy.h"
#include "net/base/io_buffer.h"

namespace chromeos {
namespace file_system_provider {
namespace {

const char kFakeFileName[] = "hello.txt";
const char kFakeFileText[] =
    "This is a testing file. Lorem ipsum dolor sit amet est.";
const size_t kFakeFileSize = sizeof(kFakeFileText) - 1u;
const char kFakeFileModificationTime[] = "Fri Apr 25 01:47:53 UTC 2014";
const char kFakeFileMimeType[] = "text/plain";

}  // namespace

const char kFakeFilePath[] = "/hello.txt";

FakeEntry::FakeEntry() {
}

FakeEntry::FakeEntry(scoped_ptr<EntryMetadata> metadata,
                     const std::string& contents)
    : metadata(metadata.Pass()), contents(contents) {
}

FakeEntry::~FakeEntry() {
}

FakeProvidedFileSystem::FakeProvidedFileSystem(
    const ProvidedFileSystemInfo& file_system_info)
    : file_system_info_(file_system_info),
      last_file_handle_(0),
      weak_ptr_factory_(this) {
  AddEntry(
      base::FilePath::FromUTF8Unsafe("/"), true, "", 0, base::Time(), "", "");

  base::Time modification_time;
  DCHECK(base::Time::FromString(kFakeFileModificationTime, &modification_time));
  AddEntry(base::FilePath::FromUTF8Unsafe(kFakeFilePath),
           false,
           kFakeFileName,
           kFakeFileSize,
           modification_time,
           kFakeFileMimeType,
           kFakeFileText);
}

FakeProvidedFileSystem::~FakeProvidedFileSystem() {}

void FakeProvidedFileSystem::AddEntry(const base::FilePath& entry_path,
                                      bool is_directory,
                                      const std::string& name,
                                      int64 size,
                                      base::Time modification_time,
                                      std::string mime_type,
                                      std::string contents) {
  DCHECK(entries_.find(entry_path) == entries_.end());
  scoped_ptr<EntryMetadata> metadata(new EntryMetadata);

  metadata->is_directory = is_directory;
  metadata->name = name;
  metadata->size = size;
  metadata->modification_time = modification_time;
  metadata->mime_type = mime_type;

  entries_[entry_path] =
      make_linked_ptr(new FakeEntry(metadata.Pass(), contents));
}

const FakeEntry* FakeProvidedFileSystem::GetEntry(
    const base::FilePath& entry_path) const {
  const Entries::const_iterator entry_it = entries_.find(entry_path);
  if (entry_it == entries_.end())
    return NULL;

  return entry_it->second.get();
}

ProvidedFileSystemInterface::AbortCallback
FakeProvidedFileSystem::RequestUnmount(
    const storage::AsyncFileUtil::StatusCallback& callback) {
  return PostAbortableTask(base::Bind(callback, base::File::FILE_OK));
}

ProvidedFileSystemInterface::AbortCallback FakeProvidedFileSystem::GetMetadata(
    const base::FilePath& entry_path,
    ProvidedFileSystemInterface::MetadataFieldMask fields,
    const ProvidedFileSystemInterface::GetMetadataCallback& callback) {
  const Entries::const_iterator entry_it = entries_.find(entry_path);

  if (entry_it == entries_.end()) {
    return PostAbortableTask(
        base::Bind(callback,
                   base::Passed(make_scoped_ptr<EntryMetadata>(NULL)),
                   base::File::FILE_ERROR_NOT_FOUND));
  }

  scoped_ptr<EntryMetadata> metadata(new EntryMetadata);
  metadata->is_directory = entry_it->second->metadata->is_directory;
  metadata->name = entry_it->second->metadata->name;
  metadata->size = entry_it->second->metadata->size;
  metadata->modification_time = entry_it->second->metadata->modification_time;
  metadata->mime_type = entry_it->second->metadata->mime_type;
  metadata->thumbnail = entry_it->second->metadata->thumbnail;

  return PostAbortableTask(
      base::Bind(callback, base::Passed(&metadata), base::File::FILE_OK));
}

ProvidedFileSystemInterface::AbortCallback
FakeProvidedFileSystem::ReadDirectory(
    const base::FilePath& directory_path,
    const storage::AsyncFileUtil::ReadDirectoryCallback& callback) {
  storage::AsyncFileUtil::EntryList entry_list;

  for (Entries::const_iterator it = entries_.begin(); it != entries_.end();
       ++it) {
    const base::FilePath file_path = it->first;
    if (file_path == directory_path || directory_path.IsParent(file_path)) {
      const EntryMetadata* const metadata = it->second->metadata.get();
      entry_list.push_back(storage::DirectoryEntry(
          metadata->name,
          metadata->is_directory ? storage::DirectoryEntry::DIRECTORY
                                 : storage::DirectoryEntry::FILE,
          metadata->size,
          metadata->modification_time));
    }
  }

  return PostAbortableTask(base::Bind(
      callback, base::File::FILE_OK, entry_list, false /* has_more */));
}

ProvidedFileSystemInterface::AbortCallback FakeProvidedFileSystem::OpenFile(
    const base::FilePath& entry_path,
    OpenFileMode mode,
    const OpenFileCallback& callback) {
  const Entries::const_iterator entry_it = entries_.find(entry_path);

  if (entry_it == entries_.end()) {
    return PostAbortableTask(base::Bind(
        callback, 0 /* file_handle */, base::File::FILE_ERROR_NOT_FOUND));
  }

  const int file_handle = ++last_file_handle_;
  opened_files_[file_handle] = entry_path;
  return PostAbortableTask(
      base::Bind(callback, file_handle, base::File::FILE_OK));
}

ProvidedFileSystemInterface::AbortCallback FakeProvidedFileSystem::CloseFile(
    int file_handle,
    const storage::AsyncFileUtil::StatusCallback& callback) {
  const OpenedFilesMap::iterator opened_file_it =
      opened_files_.find(file_handle);

  if (opened_file_it == opened_files_.end()) {
    return PostAbortableTask(
        base::Bind(callback, base::File::FILE_ERROR_NOT_FOUND));
  }

  opened_files_.erase(opened_file_it);
  return PostAbortableTask(base::Bind(callback, base::File::FILE_OK));
}

ProvidedFileSystemInterface::AbortCallback FakeProvidedFileSystem::ReadFile(
    int file_handle,
    net::IOBuffer* buffer,
    int64 offset,
    int length,
    const ProvidedFileSystemInterface::ReadChunkReceivedCallback& callback) {
  const OpenedFilesMap::iterator opened_file_it =
      opened_files_.find(file_handle);

  if (opened_file_it == opened_files_.end() ||
      opened_file_it->second.AsUTF8Unsafe() != kFakeFilePath) {
    return PostAbortableTask(
        base::Bind(callback,
                   0 /* chunk_length */,
                   false /* has_more */,
                   base::File::FILE_ERROR_INVALID_OPERATION));
  }

  const Entries::const_iterator entry_it =
      entries_.find(opened_file_it->second);
  if (entry_it == entries_.end()) {
    return PostAbortableTask(
        base::Bind(callback,
                   0 /* chunk_length */,
                   false /* has_more */,
                   base::File::FILE_ERROR_INVALID_OPERATION));
  }

  // Send the response byte by byte.
  int64 current_offset = offset;
  int current_length = length;

  // Reading behind EOF is fine, it will just return 0 bytes.
  if (current_offset >= entry_it->second->metadata->size || !current_length) {
    return PostAbortableTask(base::Bind(callback,
                                        0 /* chunk_length */,
                                        false /* has_more */,
                                        base::File::FILE_OK));
  }

  const FakeEntry* const entry = entry_it->second.get();
  std::vector<int> task_ids;
  while (current_offset < entry->metadata->size && current_length) {
    buffer->data()[current_offset - offset] = entry->contents[current_offset];
    const bool has_more =
        (current_offset + 1 < entry->metadata->size) && (current_length - 1);
    const int task_id = tracker_.PostTask(
        base::MessageLoopProxy::current().get(),
        FROM_HERE,
        base::Bind(
            callback, 1 /* chunk_length */, has_more, base::File::FILE_OK));
    task_ids.push_back(task_id);
    current_offset++;
    current_length--;
  }

  return base::Bind(&FakeProvidedFileSystem::AbortMany,
                    weak_ptr_factory_.GetWeakPtr(),
                    task_ids);
}

ProvidedFileSystemInterface::AbortCallback
FakeProvidedFileSystem::CreateDirectory(
    const base::FilePath& directory_path,
    bool recursive,
    const storage::AsyncFileUtil::StatusCallback& callback) {
  // TODO(mtomasz): Implement it once needed.
  return PostAbortableTask(base::Bind(callback, base::File::FILE_OK));
}

ProvidedFileSystemInterface::AbortCallback FakeProvidedFileSystem::DeleteEntry(
    const base::FilePath& entry_path,
    bool recursive,
    const storage::AsyncFileUtil::StatusCallback& callback) {
  // TODO(mtomasz): Implement it once needed.
  return PostAbortableTask(base::Bind(callback, base::File::FILE_OK));
}

ProvidedFileSystemInterface::AbortCallback FakeProvidedFileSystem::CreateFile(
    const base::FilePath& file_path,
    const storage::AsyncFileUtil::StatusCallback& callback) {
  const base::File::Error result = file_path.AsUTF8Unsafe() != kFakeFilePath
                                       ? base::File::FILE_ERROR_EXISTS
                                       : base::File::FILE_OK;

  return PostAbortableTask(base::Bind(callback, result));
}

ProvidedFileSystemInterface::AbortCallback FakeProvidedFileSystem::CopyEntry(
    const base::FilePath& source_path,
    const base::FilePath& target_path,
    const storage::AsyncFileUtil::StatusCallback& callback) {
  // TODO(mtomasz): Implement it once needed.
  return PostAbortableTask(base::Bind(callback, base::File::FILE_OK));
}

ProvidedFileSystemInterface::AbortCallback FakeProvidedFileSystem::MoveEntry(
    const base::FilePath& source_path,
    const base::FilePath& target_path,
    const storage::AsyncFileUtil::StatusCallback& callback) {
  // TODO(mtomasz): Implement it once needed.
  return PostAbortableTask(base::Bind(callback, base::File::FILE_OK));
}

ProvidedFileSystemInterface::AbortCallback FakeProvidedFileSystem::Truncate(
    const base::FilePath& file_path,
    int64 length,
    const storage::AsyncFileUtil::StatusCallback& callback) {
  // TODO(mtomasz): Implement it once needed.
  return PostAbortableTask(base::Bind(callback, base::File::FILE_OK));
}

ProvidedFileSystemInterface::AbortCallback FakeProvidedFileSystem::WriteFile(
    int file_handle,
    net::IOBuffer* buffer,
    int64 offset,
    int length,
    const storage::AsyncFileUtil::StatusCallback& callback) {
  const OpenedFilesMap::iterator opened_file_it =
      opened_files_.find(file_handle);

  if (opened_file_it == opened_files_.end() ||
      opened_file_it->second.AsUTF8Unsafe() != kFakeFilePath) {
    return PostAbortableTask(
        base::Bind(callback, base::File::FILE_ERROR_INVALID_OPERATION));
  }

  const Entries::iterator entry_it = entries_.find(opened_file_it->second);
  if (entry_it == entries_.end()) {
    return PostAbortableTask(
        base::Bind(callback, base::File::FILE_ERROR_INVALID_OPERATION));
  }

  FakeEntry* const entry = entry_it->second.get();
  if (offset > entry->metadata->size) {
    return PostAbortableTask(
        base::Bind(callback, base::File::FILE_ERROR_INVALID_OPERATION));
  }

  // Allocate the string size in advance.
  if (offset + length > entry->metadata->size) {
    entry->metadata->size = offset + length;
    entry->contents.resize(entry->metadata->size);
  }

  entry->contents.replace(offset, length, buffer->data(), length);

  return PostAbortableTask(base::Bind(callback, base::File::FILE_OK));
}

ProvidedFileSystemInterface::AbortCallback
FakeProvidedFileSystem::ObserveDirectory(
    const GURL& origin,
    const base::FilePath& directory_path,
    bool recursive,
    bool persistent,
    const storage::AsyncFileUtil::StatusCallback& callback) {
  // TODO(mtomasz): Implement it once needed.
  return PostAbortableTask(base::Bind(callback, base::File::FILE_OK));
}

void FakeProvidedFileSystem::UnobserveEntry(
    const GURL& origin,
    const base::FilePath& entry_path,
    bool recursive,
    const storage::AsyncFileUtil::StatusCallback& callback) {
  // TODO(mtomasz): Implement it once needed.
  callback.Run(base::File::FILE_OK);
}

const ProvidedFileSystemInfo& FakeProvidedFileSystem::GetFileSystemInfo()
    const {
  return file_system_info_;
}

RequestManager* FakeProvidedFileSystem::GetRequestManager() {
  NOTREACHED();
  return NULL;
}

ObservedEntries* FakeProvidedFileSystem::GetObservedEntries() {
  return &observed_entries_;
}

void FakeProvidedFileSystem::AddObserver(ProvidedFileSystemObserver* observer) {
  DCHECK(observer);
  observers_.AddObserver(observer);
}

void FakeProvidedFileSystem::RemoveObserver(
    ProvidedFileSystemObserver* observer) {
  DCHECK(observer);
  observers_.RemoveObserver(observer);
}

bool FakeProvidedFileSystem::Notify(
    const base::FilePath& observed_path,
    bool recursive,
    ProvidedFileSystemObserver::ChangeType change_type,
    scoped_ptr<ProvidedFileSystemObserver::Changes> changes,
    const std::string& tag) {
  NOTREACHED();
  return false;
}

ProvidedFileSystemInterface* FakeProvidedFileSystem::Create(
    Profile* profile,
    const ProvidedFileSystemInfo& file_system_info) {
  return new FakeProvidedFileSystem(file_system_info);
}

base::WeakPtr<ProvidedFileSystemInterface>
FakeProvidedFileSystem::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

ProvidedFileSystemInterface::AbortCallback
FakeProvidedFileSystem::PostAbortableTask(const base::Closure& callback) {
  const int task_id = tracker_.PostTask(
      base::MessageLoopProxy::current().get(), FROM_HERE, callback);
  return base::Bind(
      &FakeProvidedFileSystem::Abort, weak_ptr_factory_.GetWeakPtr(), task_id);
}

void FakeProvidedFileSystem::Abort(
    int task_id,
    const storage::AsyncFileUtil::StatusCallback& callback) {
  tracker_.TryCancel(task_id);
  callback.Run(base::File::FILE_OK);
}

void FakeProvidedFileSystem::AbortMany(
    const std::vector<int>& task_ids,
    const storage::AsyncFileUtil::StatusCallback& callback) {
  for (size_t i = 0; i < task_ids.size(); ++i) {
    tracker_.TryCancel(task_ids[i]);
  }
  callback.Run(base::File::FILE_OK);
}

}  // namespace file_system_provider
}  // namespace chromeos
