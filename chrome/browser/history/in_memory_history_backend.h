// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// The InMemoryHistoryBackend is a wrapper around the in-memory URL database.
// It maintains an in-memory cache of a subset of history that is required for
// low-latency operations, such as in-line autocomplete.
//
// The in-memory cache provides the following guarantees:
//  (1.) It will always contain URLRows that either have a |typed_count| > 0; or
//       that have a corresponding search term, in which case information about
//       the search term is also stored.
//  (2.) It will be an actual subset, i.e., it will contain verbatim data, and
//       will never contain more data that can be found in the main database.
//
// The InMemoryHistoryBackend is created on the history thread and passed to the
// main thread where operations can be completed synchronously. It listens for
// notifications from the "regular" history backend and keeps itself in sync.

#ifndef CHROME_BROWSER_HISTORY_IN_MEMORY_HISTORY_BACKEND_H_
#define CHROME_BROWSER_HISTORY_IN_MEMORY_HISTORY_BACKEND_H_

#include <string>

#include "base/basictypes.h"
#include "base/gtest_prod_util.h"
#include "base/memory/scoped_ptr.h"
#include "base/scoped_observer.h"
#include "components/history/core/browser/history_service_observer.h"
#include "components/history/core/browser/keyword_id.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"

class HistoryService;
class Profile;

namespace base {
class FilePath;
}

namespace history {

class InMemoryDatabase;
class URLDatabase;
class URLRow;
struct URLsDeletedDetails;
struct URLsModifiedDetails;

class InMemoryHistoryBackend : public HistoryServiceObserver,
                               public content::NotificationObserver {
 public:
  InMemoryHistoryBackend();
  ~InMemoryHistoryBackend() override;

  // Initializes the backend from the history database pointed to by the
  // full path in |history_filename|.
  bool Init(const base::FilePath& history_filename);

  // Does initialization work when this object is attached to the history
  // system on the main thread. The argument is the profile with which the
  // attached history service is under.
  void AttachToHistoryService(Profile* profile,
                              HistoryService* history_service);

  // Deletes all search terms for the specified keyword.
  void DeleteAllSearchTermsForKeyword(KeywordID keyword_id);

  // Returns the underlying database associated with this backend. The current
  // autocomplete code was written fro this, but it should probably be removed
  // so that it can deal directly with this object, rather than the DB.
  InMemoryDatabase* db() const {
    return db_.get();
  }

  // HistoryServiceObserver:
  void OnURLVisited(HistoryService* history_service,
                    ui::PageTransition transition,
                    const URLRow& row,
                    const RedirectList& redirects,
                    base::Time visit_time) override;
  void OnURLsModified(HistoryService* history_service,
                      const URLRows& changed_urls) override;
  void OnKeywordSearchTermUpdated(HistoryService* history_service,
                                  const URLRow& row,
                                  KeywordID keyword_id,
                                  const base::string16& term) override;
  void OnKeywordSearchTermDeleted(HistoryService* history_service,
                                  URLID url_id) override;

  // Notification callback.
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

 private:
  FRIEND_TEST_ALL_PREFIXES(HistoryBackendTest, DeleteAll);

  // Handler for HISTORY_URL_VISITED and HISTORY_URLS_MODIFIED.
  void OnURLVisitedOrModified(const URLRow& url_row);

  // Handler for HISTORY_URLS_DELETED.
  void OnURLsDeleted(const URLsDeletedDetails& details);

  content::NotificationRegistrar registrar_;

  scoped_ptr<InMemoryDatabase> db_;

  // The profile that this object is attached. May be NULL before
  // initialization.
  Profile* profile_;
  ScopedObserver<HistoryService, HistoryServiceObserver>
      history_service_observer_;
  HistoryService* history_service_;

  DISALLOW_COPY_AND_ASSIGN(InMemoryHistoryBackend);
};

}  // namespace history

#endif  // CHROME_BROWSER_HISTORY_IN_MEMORY_HISTORY_BACKEND_H_
