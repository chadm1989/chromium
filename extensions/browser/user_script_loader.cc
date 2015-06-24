// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/user_script_loader.h"

#include <set>
#include <string>

#include "base/version.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_types.h"
#include "content/public/browser/render_process_host.h"
#include "extensions/browser/extensions_browser_client.h"
#include "extensions/browser/notification_types.h"
#include "extensions/common/extension_messages.h"

using content::BrowserThread;
using content::BrowserContext;

namespace extensions {

namespace {

// Helper function to parse greasesmonkey headers
bool GetDeclarationValue(const base::StringPiece& line,
                         const base::StringPiece& prefix,
                         std::string* value) {
  base::StringPiece::size_type index = line.find(prefix);
  if (index == base::StringPiece::npos)
    return false;

  std::string temp(line.data() + index + prefix.length(),
                   line.length() - index - prefix.length());

  if (temp.empty() || !base::IsUnicodeWhitespace(temp[0]))
    return false;

  base::TrimWhitespaceASCII(temp, base::TRIM_ALL, value);
  return true;
}

}  // namespace

// static
bool UserScriptLoader::ParseMetadataHeader(const base::StringPiece& script_text,
                                           UserScript* script) {
  // http://wiki.greasespot.net/Metadata_block
  base::StringPiece line;
  size_t line_start = 0;
  size_t line_end = line_start;
  bool in_metadata = false;

  static const base::StringPiece kUserScriptBegin("// ==UserScript==");
  static const base::StringPiece kUserScriptEng("// ==/UserScript==");
  static const base::StringPiece kNamespaceDeclaration("// @namespace");
  static const base::StringPiece kNameDeclaration("// @name");
  static const base::StringPiece kVersionDeclaration("// @version");
  static const base::StringPiece kDescriptionDeclaration("// @description");
  static const base::StringPiece kIncludeDeclaration("// @include");
  static const base::StringPiece kExcludeDeclaration("// @exclude");
  static const base::StringPiece kMatchDeclaration("// @match");
  static const base::StringPiece kExcludeMatchDeclaration("// @exclude_match");
  static const base::StringPiece kRunAtDeclaration("// @run-at");
  static const base::StringPiece kRunAtDocumentStartValue("document-start");
  static const base::StringPiece kRunAtDocumentEndValue("document-end");
  static const base::StringPiece kRunAtDocumentIdleValue("document-idle");

  while (line_start < script_text.length()) {
    line_end = script_text.find('\n', line_start);

    // Handle the case where there is no trailing newline in the file.
    if (line_end == std::string::npos)
      line_end = script_text.length() - 1;

    line.set(script_text.data() + line_start, line_end - line_start);

    if (!in_metadata) {
      if (line.starts_with(kUserScriptBegin))
        in_metadata = true;
    } else {
      if (line.starts_with(kUserScriptEng))
        break;

      std::string value;
      if (GetDeclarationValue(line, kIncludeDeclaration, &value)) {
        // We escape some characters that MatchPattern() considers special.
        ReplaceSubstringsAfterOffset(&value, 0, "\\", "\\\\");
        ReplaceSubstringsAfterOffset(&value, 0, "?", "\\?");
        script->add_glob(value);
      } else if (GetDeclarationValue(line, kExcludeDeclaration, &value)) {
        ReplaceSubstringsAfterOffset(&value, 0, "\\", "\\\\");
        ReplaceSubstringsAfterOffset(&value, 0, "?", "\\?");
        script->add_exclude_glob(value);
      } else if (GetDeclarationValue(line, kNamespaceDeclaration, &value)) {
        script->set_name_space(value);
      } else if (GetDeclarationValue(line, kNameDeclaration, &value)) {
        script->set_name(value);
      } else if (GetDeclarationValue(line, kVersionDeclaration, &value)) {
        Version version(value);
        if (version.IsValid())
          script->set_version(version.GetString());
      } else if (GetDeclarationValue(line, kDescriptionDeclaration, &value)) {
        script->set_description(value);
      } else if (GetDeclarationValue(line, kMatchDeclaration, &value)) {
        URLPattern pattern(UserScript::ValidUserScriptSchemes());
        if (URLPattern::PARSE_SUCCESS != pattern.Parse(value))
          return false;
        script->add_url_pattern(pattern);
      } else if (GetDeclarationValue(line, kExcludeMatchDeclaration, &value)) {
        URLPattern exclude(UserScript::ValidUserScriptSchemes());
        if (URLPattern::PARSE_SUCCESS != exclude.Parse(value))
          return false;
        script->add_exclude_url_pattern(exclude);
      } else if (GetDeclarationValue(line, kRunAtDeclaration, &value)) {
        if (value == kRunAtDocumentStartValue)
          script->set_run_location(UserScript::DOCUMENT_START);
        else if (value == kRunAtDocumentEndValue)
          script->set_run_location(UserScript::DOCUMENT_END);
        else if (value == kRunAtDocumentIdleValue)
          script->set_run_location(UserScript::DOCUMENT_IDLE);
        else
          return false;
      }

      // TODO(aa): Handle more types of metadata.
    }

    line_start = line_end + 1;
  }

  // If no patterns were specified, default to @include *. This is what
  // Greasemonkey does.
  if (script->globs().empty() && script->url_patterns().is_empty())
    script->add_glob("*");

  return true;
}

UserScriptLoader::UserScriptLoader(BrowserContext* browser_context,
                                   const HostID& host_id)
    : user_scripts_(new UserScriptList()),
      clear_scripts_(false),
      ready_(false),
      pending_load_(false),
      browser_context_(browser_context),
      host_id_(host_id),
      weak_factory_(this) {
  registrar_.Add(this,
                 content::NOTIFICATION_RENDERER_PROCESS_CREATED,
                 content::NotificationService::AllBrowserContextsAndSources());
}

UserScriptLoader::~UserScriptLoader() {
  FOR_EACH_OBSERVER(Observer, observers_, OnUserScriptLoaderDestroyed(this));
}

void UserScriptLoader::AddScripts(const std::set<UserScript>& scripts) {
  for (std::set<UserScript>::const_iterator it = scripts.begin();
       it != scripts.end();
       ++it) {
    removed_scripts_.erase(*it);
    added_scripts_.insert(*it);
  }
  AttemptLoad();
}

void UserScriptLoader::AddScripts(const std::set<UserScript>& scripts,
                                  int render_process_id,
                                  int render_view_id) {
  AddScripts(scripts);
}

void UserScriptLoader::RemoveScripts(const std::set<UserScript>& scripts) {
  for (std::set<UserScript>::const_iterator it = scripts.begin();
       it != scripts.end();
       ++it) {
    added_scripts_.erase(*it);
    removed_scripts_.insert(*it);
  }
  AttemptLoad();
}

void UserScriptLoader::ClearScripts() {
  clear_scripts_ = true;
  added_scripts_.clear();
  removed_scripts_.clear();
  AttemptLoad();
}

void UserScriptLoader::Observe(int type,
                               const content::NotificationSource& source,
                               const content::NotificationDetails& details) {
  DCHECK_EQ(type, content::NOTIFICATION_RENDERER_PROCESS_CREATED);
  content::RenderProcessHost* process =
      content::Source<content::RenderProcessHost>(source).ptr();
  if (!ExtensionsBrowserClient::Get()->IsSameContext(
          browser_context_, process->GetBrowserContext()))
    return;
  if (scripts_ready()) {
    SendUpdate(process, shared_memory_.get(),
               std::set<HostID>());  // Include all hosts.
  }
}

bool UserScriptLoader::ScriptsMayHaveChanged() const {
  // Scripts may have changed if there are scripts added, scripts removed, or
  // if scripts were cleared and either:
  // (1) A load is in progress (which may result in a non-zero number of
  //     scripts that need to be cleared), or
  // (2) The current set of scripts is non-empty (so they need to be cleared).
  return (added_scripts_.size() ||
          removed_scripts_.size() ||
          (clear_scripts_ &&
           (is_loading() || user_scripts_->size())));
}

void UserScriptLoader::AttemptLoad() {
  if (ready_ && ScriptsMayHaveChanged()) {
    if (is_loading())
      pending_load_ = true;
    else
      StartLoad();
  }
}

void UserScriptLoader::StartLoad() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(!is_loading());

  // If scripts were marked for clearing before adding and removing, then clear
  // them.
  if (clear_scripts_) {
    user_scripts_->clear();
  } else {
    for (UserScriptList::iterator it = user_scripts_->begin();
         it != user_scripts_->end();) {
      if (removed_scripts_.count(*it))
        it = user_scripts_->erase(it);
      else
        ++it;
    }
  }

  user_scripts_->insert(
      user_scripts_->end(), added_scripts_.begin(), added_scripts_.end());

  std::set<int> added_script_ids;
  for (std::set<UserScript>::const_iterator it = added_scripts_.begin();
       it != added_scripts_.end();
       ++it) {
    added_script_ids.insert(it->id());
  }

  // Expand |changed_hosts_| for OnScriptsLoaded, which will use it in
  // its IPC message. This must be done before we clear |added_scripts_| and
  // |removed_scripts_| below.
  std::set<UserScript> changed_scripts(added_scripts_);
  changed_scripts.insert(removed_scripts_.begin(), removed_scripts_.end());
  for (const UserScript& script : changed_scripts)
    changed_hosts_.insert(script.host_id());

  LoadScripts(user_scripts_.Pass(), changed_hosts_, added_script_ids,
              base::Bind(&UserScriptLoader::OnScriptsLoaded,
                         weak_factory_.GetWeakPtr()));

  clear_scripts_ = false;
  added_scripts_.clear();
  removed_scripts_.clear();
  user_scripts_.reset();
}

// static
scoped_ptr<base::SharedMemory> UserScriptLoader::Serialize(
    const UserScriptList& scripts) {
  base::Pickle pickle;
  pickle.WriteSizeT(scripts.size());
  for (const UserScript& script : scripts) {
    // TODO(aa): This can be replaced by sending content script metadata to
    // renderers along with other extension data in ExtensionMsg_Loaded.
    // See crbug.com/70516.
    script.Pickle(&pickle);
    // Write scripts as 'data' so that we can read it out in the slave without
    // allocating a new string.
    for (const UserScript::File& script_file : script.js_scripts()) {
      base::StringPiece contents = script_file.GetContent();
      pickle.WriteData(contents.data(), contents.length());
    }
    for (const UserScript::File& script_file : script.css_scripts()) {
      base::StringPiece contents = script_file.GetContent();
      pickle.WriteData(contents.data(), contents.length());
    }
  }

  // Create the shared memory object.
  base::SharedMemory shared_memory;

  base::SharedMemoryCreateOptions options;
  options.size = pickle.size();
  options.share_read_only = true;
  if (!shared_memory.Create(options))
    return scoped_ptr<base::SharedMemory>();

  if (!shared_memory.Map(pickle.size()))
    return scoped_ptr<base::SharedMemory>();

  // Copy the pickle to shared memory.
  memcpy(shared_memory.memory(), pickle.data(), pickle.size());

  base::SharedMemoryHandle readonly_handle;
  if (!shared_memory.ShareReadOnlyToProcess(base::GetCurrentProcessHandle(),
                                            &readonly_handle))
    return scoped_ptr<base::SharedMemory>();

  return make_scoped_ptr(new base::SharedMemory(readonly_handle,
                                                /*read_only=*/true));
}

void UserScriptLoader::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void UserScriptLoader::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void UserScriptLoader::SetReady(bool ready) {
  bool was_ready = ready_;
  ready_ = ready;
  if (ready_ && !was_ready)
    AttemptLoad();
}

void UserScriptLoader::OnScriptsLoaded(
    scoped_ptr<UserScriptList> user_scripts,
    scoped_ptr<base::SharedMemory> shared_memory) {
  user_scripts_.reset(user_scripts.release());
  if (pending_load_) {
    // While we were loading, there were further changes. Don't bother
    // notifying about these scripts and instead just immediately reload.
    pending_load_ = false;
    StartLoad();
    return;
  }

  if (shared_memory.get() == NULL) {
    // This can happen if we run out of file descriptors.  In that case, we
    // have a choice between silently omitting all user scripts for new tabs,
    // by nulling out shared_memory_, or only silently omitting new ones by
    // leaving the existing object in place. The second seems less bad, even
    // though it removes the possibility that freeing the shared memory block
    // would open up enough FDs for long enough for a retry to succeed.

    // Pretend the extension change didn't happen.
    return;
  }

  // We've got scripts ready to go.
  shared_memory_.reset(shared_memory.release());

  for (content::RenderProcessHost::iterator i(
           content::RenderProcessHost::AllHostsIterator());
       !i.IsAtEnd(); i.Advance()) {
    SendUpdate(i.GetCurrentValue(), shared_memory_.get(), changed_hosts_);
  }
  changed_hosts_.clear();

  // TODO(hanxi): Remove the NOTIFICATION_USER_SCRIPTS_UPDATED.
  content::NotificationService::current()->Notify(
      extensions::NOTIFICATION_USER_SCRIPTS_UPDATED,
      content::Source<BrowserContext>(browser_context_),
      content::Details<base::SharedMemory>(shared_memory_.get()));
  FOR_EACH_OBSERVER(Observer, observers_, OnScriptsLoaded(this));
}

void UserScriptLoader::SendUpdate(content::RenderProcessHost* process,
                                  base::SharedMemory* shared_memory,
                                  const std::set<HostID>& changed_hosts) {
  // Don't allow injection of non-whitelisted extensions' content scripts
  // into <webview>.
  bool whitelisted_only = process->IsForGuestsOnly() && host_id().id().empty();

  // Make sure we only send user scripts to processes in our browser_context.
  if (!ExtensionsBrowserClient::Get()->IsSameContext(
          browser_context_, process->GetBrowserContext()))
    return;

  // If the process is being started asynchronously, early return.  We'll end up
  // calling InitUserScripts when it's created which will call this again.
  base::ProcessHandle handle = process->GetHandle();
  if (!handle)
    return;

  base::SharedMemoryHandle handle_for_process;
  if (!shared_memory->ShareToProcess(handle, &handle_for_process))
    return;  // This can legitimately fail if the renderer asserts at startup.

  if (base::SharedMemory::IsHandleValid(handle_for_process)) {
    process->Send(new ExtensionMsg_UpdateUserScripts(
        handle_for_process, host_id(), changed_hosts, whitelisted_only));
  }
}

}  // namespace extensions
