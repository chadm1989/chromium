// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_CACHE_STORAGE_CACHE_STORAGE_BLOB_TO_DISK_CACHE_H_
#define CONTENT_BROWSER_CACHE_STORAGE_CACHE_STORAGE_BLOB_TO_DISK_CACHE_H_

#include "base/callback.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"
#include "content/common/content_export.h"
#include "net/disk_cache/disk_cache.h"
#include "net/url_request/url_request.h"

namespace net {
class IOBufferWithSize;
class URLRequestContextGetter;
}

namespace storage {
class BlobDataHandle;
}

namespace content {

// Streams data from a blob and writes it to a given disk_cache::Entry.
class CONTENT_EXPORT CacheStorageBlobToDiskCache
    : public net::URLRequest::Delegate {
 public:
  using EntryAndBoolCallback =
      base::Callback<void(disk_cache::ScopedEntryPtr, bool)>;

  // The buffer size used for reading from blobs and writing to disk cache.
  static const int kBufferSize;

  CacheStorageBlobToDiskCache();
  ~CacheStorageBlobToDiskCache() override;

  // Writes the body of |blob_data_handle| to |entry| with index
  // |disk_cache_body_index|. |entry| is passed to the callback once complete.
  // Only call this once per instantiation of CacheStorageBlobToDiskCache.
  void StreamBlobToCache(
      disk_cache::ScopedEntryPtr entry,
      int disk_cache_body_index,
      const scoped_refptr<net::URLRequestContextGetter>& request_context_getter,
      scoped_ptr<storage::BlobDataHandle> blob_data_handle,
      const EntryAndBoolCallback& callback);

  // net::URLRequest::Delegate overrides for reading blobs.
  void OnResponseStarted(net::URLRequest* request) override;
  void OnReadCompleted(net::URLRequest* request, int bytes_read) override;
  void OnReceivedRedirect(net::URLRequest* request,
                          const net::RedirectInfo& redirect_info,
                          bool* defer_redirect) override;
  void OnAuthRequired(net::URLRequest* request,
                      net::AuthChallengeInfo* auth_info) override;
  void OnCertificateRequested(
      net::URLRequest* request,
      net::SSLCertRequestInfo* cert_request_info) override;
  void OnSSLCertificateError(net::URLRequest* request,
                             const net::SSLInfo& ssl_info,
                             bool fatal) override;
  void OnBeforeNetworkStart(net::URLRequest* request, bool* defer) override;

 private:
  void ReadFromBlob();
  void DidWriteDataToEntry(int expected_bytes, int rv);

  int cache_entry_offset_;
  disk_cache::ScopedEntryPtr entry_;
  int disk_cache_body_index_;
  scoped_ptr<net::URLRequest> blob_request_;
  EntryAndBoolCallback callback_;
  scoped_refptr<net::IOBufferWithSize> buffer_;

  base::WeakPtrFactory<CacheStorageBlobToDiskCache> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(CacheStorageBlobToDiskCache);
};

}  // namespace content

#endif  // CONTENT_BROWSER_CACHE_STORAGE_CACHE_STORAGE_BLOB_TO_DISK_CACHE_H_
