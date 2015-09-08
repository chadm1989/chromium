// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/shell/browser/shell_nacl_browser_delegate.h"

#include <string>

#include "base/base_paths.h"
#include "base/command_line.h"
#include "base/path_service.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/site_instance.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/info_map.h"
#include "extensions/browser/process_manager.h"
#include "extensions/common/constants.h"
#include "extensions/common/extension.h"
#include "extensions/common/url_pattern.h"
#include "extensions/shell/common/version.h"  // Generated file.
#include "url/gurl.h"

using content::BrowserContext;
using content::BrowserThread;
using content::BrowserPpapiHost;

namespace extensions {
namespace {

// Handles an extension's NaCl process transitioning in or out of idle state by
// relaying the state to the extension's process manager. See Chrome's
// NaClBrowserDelegateImpl for another example.
void OnKeepaliveOnUIThread(
    const BrowserPpapiHost::OnKeepaliveInstanceData& instance_data,
    const base::FilePath& profile_data_directory) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  // Only one instance will exist for NaCl embeds, even when more than one
  // embed of the same plugin exists on the same page.
  DCHECK(instance_data.size() == 1);
  if (instance_data.size() < 1)
    return;

  ProcessManager::OnKeepaliveFromPlugin(instance_data[0].render_process_id,
                                        instance_data[0].render_frame_id,
                                        instance_data[0].document_url.host());
}

// Calls OnKeepaliveOnUIThread on UI thread.
void OnKeepalive(const BrowserPpapiHost::OnKeepaliveInstanceData& instance_data,
                 const base::FilePath& profile_data_directory) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  BrowserThread::PostTask(
      BrowserThread::UI,
      FROM_HERE,
      base::Bind(
          &OnKeepaliveOnUIThread, instance_data, profile_data_directory));
}

}  // namespace

ShellNaClBrowserDelegate::ShellNaClBrowserDelegate(BrowserContext* context)
    : browser_context_(context) {
  DCHECK(browser_context_);
}

ShellNaClBrowserDelegate::~ShellNaClBrowserDelegate() {
}

void ShellNaClBrowserDelegate::ShowMissingArchInfobar(int render_process_id,
                                                      int render_view_id) {
  // app_shell does not have infobars.
  LOG(ERROR) << "Missing architecture for pid " << render_process_id;
}

bool ShellNaClBrowserDelegate::DialogsAreSuppressed() {
  return false;
}

bool ShellNaClBrowserDelegate::GetCacheDirectory(base::FilePath* cache_dir) {
  // Just use the general cache directory, not a subdirectory like Chrome does.
#if defined(OS_POSIX)
  return PathService::Get(base::DIR_CACHE, cache_dir);
#elif defined(OS_WIN)
  // TODO(yoz): Find an appropriate persistent directory to use here.
  return PathService::Get(base::DIR_TEMP, cache_dir);
#endif
}

bool ShellNaClBrowserDelegate::GetPluginDirectory(base::FilePath* plugin_dir) {
  // On Posix, plugins are in the module directory.
  return PathService::Get(base::DIR_MODULE, plugin_dir);
}

bool ShellNaClBrowserDelegate::GetPnaclDirectory(base::FilePath* pnacl_dir) {
  // On Posix, the pnacl directory is inside the plugin directory.
  base::FilePath plugin_dir;
  if (!GetPluginDirectory(&plugin_dir))
    return false;
  *pnacl_dir = plugin_dir.Append(FILE_PATH_LITERAL("pnacl"));
  return true;
}

bool ShellNaClBrowserDelegate::GetUserDirectory(base::FilePath* user_dir) {
  base::FilePath path = browser_context_->GetPath();
  if (!path.empty()) {
    *user_dir = path;
    return true;
  }
  return false;
}

std::string ShellNaClBrowserDelegate::GetVersionString() const {
  // A version change triggers an update of the NaCl validation caches.
  // Example version: "39.0.2129.0 (290550)".
  return PRODUCT_VERSION " (" LAST_CHANGE ")";
}

ppapi::host::HostFactory* ShellNaClBrowserDelegate::CreatePpapiHostFactory(
    content::BrowserPpapiHost* ppapi_host) {
  return NULL;
}

void ShellNaClBrowserDelegate::SetDebugPatterns(
    const std::string& debug_patterns) {
  // No debugger support. Developers should use Chrome for debugging.
}

bool ShellNaClBrowserDelegate::URLMatchesDebugPatterns(
    const GURL& manifest_url) {
  // No debugger support. Developers should use Chrome for debugging.
  return false;
}

// This function is security sensitive.  Be sure to check with a security
// person before you modify it.
// TODO(jamescook): Refactor this code into the extensions module so it can
// be shared with Chrome's NaClBrowserDelegateImpl. http://crbug.com/403017
bool ShellNaClBrowserDelegate::MapUrlToLocalFilePath(
    const GURL& file_url,
    bool use_blocking_api,
    const base::FilePath& profile_directory,
    base::FilePath* file_path) {
  scoped_refptr<InfoMap> info_map =
      ExtensionSystem::Get(browser_context_)->info_map();
  // Check that the URL is recognized by the extension system.
  const Extension* extension =
      info_map->extensions().GetExtensionOrAppByURL(file_url);
  if (!extension)
    return false;

  // This is a short-cut which avoids calling a blocking file operation
  // (GetFilePath()), so that this can be called on the IO thread. It only
  // handles a subset of the urls.
  if (!use_blocking_api) {
    if (file_url.SchemeIs(kExtensionScheme)) {
      std::string path = file_url.path();
      base::TrimString(path, "/", &path);  // Remove first slash
      *file_path = extension->path().AppendASCII(path);
      return true;
    }
    return false;
  }

  // Check that the URL references a resource in the extension.
  // NOTE: app_shell does not support shared modules.
  ExtensionResource resource = extension->GetResource(file_url.path());
  if (resource.empty())
    return false;

  // GetFilePath is a blocking function call.
  const base::FilePath resource_file_path = resource.GetFilePath();
  if (resource_file_path.empty())
    return false;

  *file_path = resource_file_path;
  return true;
}

content::BrowserPpapiHost::OnKeepaliveCallback
ShellNaClBrowserDelegate::GetOnKeepaliveCallback() {
  return base::Bind(&OnKeepalive);
}

bool ShellNaClBrowserDelegate::IsNonSfiModeAllowed(
    const base::FilePath& profile_directory,
    const GURL& manifest_url) {
  return false;
}

}  // namespace extensions
