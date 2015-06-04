// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/extension_gcm_app_handler.h"

#include "base/bind.h"
#include "base/lazy_instance.h"
#include "base/location.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/extensions/api/gcm/gcm_api.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/services/gcm/gcm_profile_service.h"
#include "chrome/browser/services/gcm/gcm_profile_service_factory.h"
#include "chrome/browser/services/gcm/instance_id/instance_id_profile_service.h"
#include "chrome/browser/services/gcm/instance_id/instance_id_profile_service_factory.h"
#include "components/gcm_driver/gcm_driver.h"
#include "components/gcm_driver/instance_id/instance_id_driver.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"
#include "extensions/common/extension.h"
#include "extensions/common/permissions/permissions_data.h"

namespace extensions {

namespace {

const char kDummyAppId[] = "extension.guard.dummy.id";

base::LazyInstance<BrowserContextKeyedAPIFactory<ExtensionGCMAppHandler> >
    g_factory = LAZY_INSTANCE_INITIALIZER;

bool IsGCMPermissionEnabled(const Extension* extension) {
  return extension->permissions_data()->HasAPIPermission(APIPermission::kGcm);
}

}  // namespace


// static
BrowserContextKeyedAPIFactory<ExtensionGCMAppHandler>*
ExtensionGCMAppHandler::GetFactoryInstance() {
  return g_factory.Pointer();
}

ExtensionGCMAppHandler::ExtensionGCMAppHandler(content::BrowserContext* context)
    : profile_(Profile::FromBrowserContext(context)),
      extension_registry_observer_(this),
      weak_factory_(this) {
  extension_registry_observer_.Add(ExtensionRegistry::Get(profile_));
  js_event_router_.reset(new extensions::GcmJsEventRouter(profile_));
}

ExtensionGCMAppHandler::~ExtensionGCMAppHandler() {
  const ExtensionSet& enabled_extensions =
      ExtensionRegistry::Get(profile_)->enabled_extensions();
  for (ExtensionSet::const_iterator extension = enabled_extensions.begin();
       extension != enabled_extensions.end();
       ++extension) {
    if (IsGCMPermissionEnabled(extension->get()))
      GetGCMDriver()->RemoveAppHandler((*extension)->id());
  }
}

void ExtensionGCMAppHandler::ShutdownHandler() {
  js_event_router_.reset();
}

void ExtensionGCMAppHandler::OnMessage(
    const std::string& app_id,
    const gcm::GCMClient::IncomingMessage& message) {
  js_event_router_->OnMessage(app_id, message);
}

void ExtensionGCMAppHandler::OnMessagesDeleted(const std::string& app_id) {
  js_event_router_->OnMessagesDeleted(app_id);
}

void ExtensionGCMAppHandler::OnSendError(
    const std::string& app_id,
    const gcm::GCMClient::SendErrorDetails& send_error_details) {
  js_event_router_->OnSendError(app_id, send_error_details);
}

void ExtensionGCMAppHandler::OnSendAcknowledged(
    const std::string& app_id,
    const std::string& message_id) {
  // This event is not exposed to JS API. It terminates here.
}

void ExtensionGCMAppHandler::OnExtensionLoaded(
    content::BrowserContext* browser_context,
    const Extension* extension) {
  if (IsGCMPermissionEnabled(extension))
    AddAppHandler(extension->id());
}

void ExtensionGCMAppHandler::OnExtensionUnloaded(
    content::BrowserContext* browser_context,
    const Extension* extension,
    UnloadedExtensionInfo::Reason reason) {
  if (!IsGCMPermissionEnabled(extension))
    return;

  if (reason == UnloadedExtensionInfo::REASON_UPDATE &&
      GetGCMDriver()->app_handlers().size() >= 1) {
    // When the extension is being updated, it will be first unloaded and then
    // loaded again by ExtensionService::AddExtension. If the app handler for
    // this extension is the only handler, removing it and adding it again will
    // cause the GCM service being stopped and restarted unnecessarily. To work
    // around this, we add a dummy app handler to guard against it. This dummy
    // app handler will be removed once the extension loading logic is done.
    //
    // Note that this dummy app handler is added when there is at least one
    // handler. This is because there might be a built-in app handler, like
    // GCMAccountMapper, which is automatically added and removed by
    // GCMDriverDesktop.
    //
    // Also note that the GCM message routing will not be interruptted during
    // the update process since unloading and reloading extension are done in
    // the single function ExtensionService::AddExtension.
    AddDummyAppHandler();

    base::MessageLoop::current()->PostTask(
        FROM_HERE,
        base::Bind(&ExtensionGCMAppHandler::RemoveDummyAppHandler,
                   weak_factory_.GetWeakPtr()));
  }

  // When the extention is being uninstalled, it will be unloaded first. We
  // should not remove the app handler in this case and it will be handled
  // in OnExtensionUninstalled.
  if (reason != UnloadedExtensionInfo::REASON_UNINSTALL)
    RemoveAppHandler(extension->id());
}

void ExtensionGCMAppHandler::OnExtensionUninstalled(
    content::BrowserContext* browser_context,
    const Extension* extension,
    extensions::UninstallReason reason) {
  if (IsGCMPermissionEnabled(extension)) {
    // Let's first remove InstanceID data. GCM unregistration will be triggered
    // after the asynchronous call is returned in OnDeleteIDCompleted.
    GetInstanceIDDriver()->GetInstanceID(extension->id())->DeleteID(
        base::Bind(&ExtensionGCMAppHandler::OnDeleteIDCompleted,
                   weak_factory_.GetWeakPtr(),
                   extension->id()));
  }
}

void ExtensionGCMAppHandler::AddDummyAppHandler() {
  AddAppHandler(kDummyAppId);
}

void ExtensionGCMAppHandler::RemoveDummyAppHandler() {
  RemoveAppHandler(kDummyAppId);
}

gcm::GCMDriver* ExtensionGCMAppHandler::GetGCMDriver() const {
  return gcm::GCMProfileServiceFactory::GetForProfile(profile_)->driver();
}

instance_id::InstanceIDDriver* ExtensionGCMAppHandler::GetInstanceIDDriver()
    const {
  return instance_id::InstanceIDProfileServiceFactory::GetForProfile(profile_)->
      driver();
}

void ExtensionGCMAppHandler::OnUnregisterCompleted(
    const std::string& app_id, gcm::GCMClient::Result result) {
  RemoveAppHandler(app_id);
}

void ExtensionGCMAppHandler::OnDeleteIDCompleted(
    const std::string& app_id, instance_id::InstanceID::Result result) {
  GetGCMDriver()->Unregister(
      app_id,
      base::Bind(&ExtensionGCMAppHandler::OnUnregisterCompleted,
                 weak_factory_.GetWeakPtr(),
                 app_id));

  // InstanceIDDriver::RemoveInstanceID will delete the InstanceID itself.
  // Postpone to do it outside this calling context to avoid any risk to
  // the caller.
  base::MessageLoop::current()->PostTask(
        FROM_HERE,
        base::Bind(&ExtensionGCMAppHandler::RemoveInstanceID,
                   weak_factory_.GetWeakPtr(),
                   app_id));
}

void ExtensionGCMAppHandler::RemoveInstanceID(const std::string& app_id) {
  GetInstanceIDDriver()->RemoveInstanceID(app_id);
}

void ExtensionGCMAppHandler::AddAppHandler(const std::string& app_id) {
  GetGCMDriver()->AddAppHandler(app_id, this);
}

void ExtensionGCMAppHandler::RemoveAppHandler(const std::string& app_id) {
  GetGCMDriver()->RemoveAppHandler(app_id);
}

}  // namespace extensions
