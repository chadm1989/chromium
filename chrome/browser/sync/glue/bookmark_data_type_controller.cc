// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sync/glue/bookmark_data_type_controller.h"

#include "base/metrics/histogram.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/sync/glue/chrome_report_unrecoverable_error.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/history/core/browser/history_service.h"
#include "components/sync_driver/sync_api_component_factory.h"
#include "components/sync_driver/sync_client.h"
#include "content/public/browser/browser_thread.h"

using bookmarks::BookmarkModel;
using content::BrowserThread;

namespace browser_sync {

BookmarkDataTypeController::BookmarkDataTypeController(
    sync_driver::SyncClient* sync_client)
    : FrontendDataTypeController(
          BrowserThread::GetMessageLoopProxyForThread(BrowserThread::UI),
          base::Bind(&ChromeReportUnrecoverableError),
          sync_client),
      history_service_observer_(this),
      bookmark_model_observer_(this) {
}

syncer::ModelType BookmarkDataTypeController::type() const {
  return syncer::BOOKMARKS;
}

BookmarkDataTypeController::~BookmarkDataTypeController() {
}

bool BookmarkDataTypeController::StartModels() {
  if (!DependentsLoaded()) {
    BookmarkModel* bookmark_model = sync_client_->GetBookmarkModel();
    bookmark_model_observer_.Add(bookmark_model);
    history::HistoryService* history_service =
        sync_client_->GetHistoryService();
    history_service_observer_.Add(history_service);
    return false;
  }
  return true;
}

void BookmarkDataTypeController::CleanUpState() {
  history_service_observer_.RemoveAll();
  bookmark_model_observer_.RemoveAll();
}

void BookmarkDataTypeController::CreateSyncComponents() {
  sync_driver::SyncApiComponentFactory::SyncComponents sync_components =
      sync_client_->GetSyncApiComponentFactory()->CreateBookmarkSyncComponents(
          sync_client_->GetSyncService(), this);
  set_model_associator(sync_components.model_associator);
  set_change_processor(sync_components.change_processor);
}

void BookmarkDataTypeController::BookmarkModelChanged() {
}

void BookmarkDataTypeController::BookmarkModelLoaded(BookmarkModel* model,
                                                     bool ids_reassigned) {
  DCHECK(model->loaded());
  bookmark_model_observer_.RemoveAll();

  if (!DependentsLoaded())
    return;

  history_service_observer_.RemoveAll();
  OnModelLoaded();
}

void BookmarkDataTypeController::BookmarkModelBeingDeleted(
    BookmarkModel* model) {
  CleanUpState();
}

// Check that both the bookmark model and the history service (for favicons)
// are loaded.
bool BookmarkDataTypeController::DependentsLoaded() {
  BookmarkModel* bookmark_model = sync_client_->GetBookmarkModel();
  if (!bookmark_model || !bookmark_model->loaded())
    return false;

  history::HistoryService* history_service = sync_client_->GetHistoryService();
  if (!history_service || !history_service->BackendLoaded())
    return false;

  // All necessary services are loaded.
  return true;
}

void BookmarkDataTypeController::OnHistoryServiceLoaded(
    history::HistoryService* service) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK_EQ(state_, MODEL_STARTING);
  history_service_observer_.RemoveAll();

  if (!DependentsLoaded())
    return;

  bookmark_model_observer_.RemoveAll();
  OnModelLoaded();
}

void BookmarkDataTypeController::HistoryServiceBeingDeleted(
    history::HistoryService* history_service) {
  CleanUpState();
}

}  // namespace browser_sync
