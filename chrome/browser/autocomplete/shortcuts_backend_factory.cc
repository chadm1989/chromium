// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/autocomplete/shortcuts_backend_factory.h"

#include "base/prefs/pref_service.h"
#include "chrome/browser/autocomplete/shortcuts_backend.h"
#include "chrome/browser/autocomplete/shortcuts_extensions_manager.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

namespace {
#if defined(ENABLE_EXTENSIONS)
const char kShortcutsExtensionsManagerKey[] = "ShortcutsExtensionsManager";
#endif
}

// static
scoped_refptr<ShortcutsBackend> ShortcutsBackendFactory::GetForProfile(
    Profile* profile) {
  return static_cast<ShortcutsBackend*>(
      GetInstance()->GetServiceForBrowserContext(profile, true).get());
}

// static
scoped_refptr<ShortcutsBackend> ShortcutsBackendFactory::GetForProfileIfExists(
    Profile* profile) {
  return static_cast<ShortcutsBackend*>(
      GetInstance()->GetServiceForBrowserContext(profile, false).get());
}

// static
ShortcutsBackendFactory* ShortcutsBackendFactory::GetInstance() {
  return Singleton<ShortcutsBackendFactory>::get();
}

// static
scoped_refptr<RefcountedKeyedService>
ShortcutsBackendFactory::BuildProfileForTesting(
    content::BrowserContext* profile) {
  return CreateShortcutsBackend(Profile::FromBrowserContext(profile), false);
}

// static
scoped_refptr<RefcountedKeyedService>
ShortcutsBackendFactory::BuildProfileNoDatabaseForTesting(
    content::BrowserContext* profile) {
  return CreateShortcutsBackend(Profile::FromBrowserContext(profile), true);
}

ShortcutsBackendFactory::ShortcutsBackendFactory()
    : RefcountedBrowserContextKeyedServiceFactory(
        "ShortcutsBackend",
        BrowserContextDependencyManager::GetInstance()) {
  DependsOn(HistoryServiceFactory::GetInstance());
}

ShortcutsBackendFactory::~ShortcutsBackendFactory() {}

scoped_refptr<RefcountedKeyedService>
ShortcutsBackendFactory::BuildServiceInstanceFor(
    content::BrowserContext* profile) const {
  return CreateShortcutsBackend(Profile::FromBrowserContext(profile), false);
}

bool ShortcutsBackendFactory::ServiceIsNULLWhileTesting() const {
  return true;
}

void ShortcutsBackendFactory::BrowserContextShutdown(
    content::BrowserContext* context) {
#if defined(ENABLE_EXTENSIONS)
  context->RemoveUserData(kShortcutsExtensionsManagerKey);
#endif

  RefcountedBrowserContextKeyedServiceFactory::BrowserContextShutdown(context);
}

// static
scoped_refptr<ShortcutsBackend> ShortcutsBackendFactory::CreateShortcutsBackend(
    Profile* profile,
    bool suppress_db) {
  scoped_refptr<ShortcutsBackend> backend(
      new ShortcutsBackend(profile, suppress_db));
#if defined(ENABLE_EXTENSIONS)
  ShortcutsExtensionsManager* extensions_manager =
      new ShortcutsExtensionsManager(profile);
  profile->SetUserData(kShortcutsExtensionsManagerKey, extensions_manager);
#endif
  return backend->Init() ? backend : nullptr;
}
