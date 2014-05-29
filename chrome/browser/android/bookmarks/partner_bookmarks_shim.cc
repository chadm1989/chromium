// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/bookmarks/partner_bookmarks_shim.h"

#include "base/lazy_instance.h"
#include "base/prefs/pref_service.h"
#include "base/values.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/pref_names.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"

using content::BrowserThread;

namespace {

// PartnerModelKeeper is used as a singleton to store an immutable hierarchy
// of partner bookmarks.  The hierarchy is retrieved from the partner bookmarks
// provider and doesn't depend on the user profile.
// The retrieved hierarchy persists
// PartnerBookmarksShim is responsible to applying and storing the user changes
// (deletions/renames) in the user profile, thus keeping the hierarchy intact.
struct PartnerModelKeeper {
  scoped_ptr<BookmarkNode> partner_bookmarks_root;
  bool loaded;

  PartnerModelKeeper()
    : loaded(false) {}
};

base::LazyInstance<PartnerModelKeeper> g_partner_model_keeper =
    LAZY_INSTANCE_INITIALIZER;

const void* kPartnerBookmarksShimUserDataKey =
    &kPartnerBookmarksShimUserDataKey;

// Dictionary keys for entries in the kPartnerBookmarksMapping pref.
static const char kMappingUrl[] = "url";
static const char kMappingProviderTitle[] = "provider_title";
static const char kMappingTitle[] = "mapped_title";

static bool g_disable_partner_bookmarks_editing = false;

}  // namespace

// static
PartnerBookmarksShim* PartnerBookmarksShim::BuildForBrowserContext(
    content::BrowserContext* browser_context) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  PartnerBookmarksShim* data =
      reinterpret_cast<PartnerBookmarksShim*>(
          browser_context->GetUserData(kPartnerBookmarksShimUserDataKey));
  if (data)
    return data;

  data = new PartnerBookmarksShim(
      Profile::FromBrowserContext(browser_context)->GetPrefs());
  browser_context->SetUserData(kPartnerBookmarksShimUserDataKey, data);
  data->ReloadNodeMapping();
  return data;
}

// static
void PartnerBookmarksShim::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterListPref(
      prefs::kPartnerBookmarkMappings,
      user_prefs::PrefRegistrySyncable::UNSYNCABLE_PREF);
}

// static
void PartnerBookmarksShim::DisablePartnerBookmarksEditing() {
  g_disable_partner_bookmarks_editing = true;
}

bool PartnerBookmarksShim::IsLoaded() const {
  return g_partner_model_keeper.Get().loaded;
}

bool PartnerBookmarksShim::HasPartnerBookmarks() const {
  DCHECK(IsLoaded());
  return g_partner_model_keeper.Get().partner_bookmarks_root.get() != NULL;
}

bool PartnerBookmarksShim::IsReachable(const BookmarkNode* node) const {
  DCHECK(IsPartnerBookmark(node));
  if (!HasPartnerBookmarks())
    return false;
  if (!g_disable_partner_bookmarks_editing) {
    for (const BookmarkNode* i = node; i != NULL; i = i->parent()) {
      const NodeRenamingMapKey key(i->url(), i->GetTitle());
      NodeRenamingMap::const_iterator remap = node_rename_remove_map_.find(key);
      if (remap != node_rename_remove_map_.end() && remap->second.empty())
        return false;
    }
  }
  return true;
}

bool PartnerBookmarksShim::IsEditable(const BookmarkNode* node) const {
  DCHECK(IsPartnerBookmark(node));
  if (!HasPartnerBookmarks())
    return false;
  if (g_disable_partner_bookmarks_editing)
    return false;
  return true;
}

void PartnerBookmarksShim::RemoveBookmark(const BookmarkNode* node) {
  DCHECK(IsEditable(node));
  RenameBookmark(node, base::string16());
}

void PartnerBookmarksShim::RenameBookmark(const BookmarkNode* node,
                                          const base::string16& title) {
  DCHECK(IsEditable(node));
  const NodeRenamingMapKey key(node->url(), node->GetTitle());
  node_rename_remove_map_[key] = title;
  SaveNodeMapping();
  FOR_EACH_OBSERVER(PartnerBookmarksShim::Observer, observers_,
                    PartnerShimChanged(this));
}

void PartnerBookmarksShim::AddObserver(
    PartnerBookmarksShim::Observer* observer) {
  observers_.AddObserver(observer);
}

void PartnerBookmarksShim::RemoveObserver(
    PartnerBookmarksShim::Observer* observer) {
  observers_.RemoveObserver(observer);
}

const BookmarkNode* PartnerBookmarksShim::GetNodeByID(int64 id) const {
  DCHECK(IsLoaded());
  if (!HasPartnerBookmarks())
    return NULL;
  return GetNodeByID(GetPartnerBookmarksRoot(), id);
}

base::string16 PartnerBookmarksShim::GetTitle(const BookmarkNode* node) const {
  DCHECK(node);
  DCHECK(IsPartnerBookmark(node));

  if (!g_disable_partner_bookmarks_editing) {
    const NodeRenamingMapKey key(node->url(), node->GetTitle());
    NodeRenamingMap::const_iterator i = node_rename_remove_map_.find(key);
    if (i != node_rename_remove_map_.end())
      return i->second;
  }

  return node->GetTitle();
}

bool PartnerBookmarksShim::IsPartnerBookmark(const BookmarkNode* node) const {
  DCHECK(IsLoaded());
  if (!HasPartnerBookmarks())
    return false;
  const BookmarkNode* parent = node;
  while (parent) {
    if (parent == GetPartnerBookmarksRoot())
      return true;
    parent = parent->parent();
  }
  return false;
}

const BookmarkNode* PartnerBookmarksShim::GetPartnerBookmarksRoot() const {
  return g_partner_model_keeper.Get().partner_bookmarks_root.get();
}

void PartnerBookmarksShim::SetPartnerBookmarksRoot(BookmarkNode* root_node) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  g_partner_model_keeper.Get().partner_bookmarks_root.reset(root_node);
  g_partner_model_keeper.Get().loaded = true;
  FOR_EACH_OBSERVER(PartnerBookmarksShim::Observer, observers_,
                    PartnerShimLoaded(this));
}

PartnerBookmarksShim::NodeRenamingMapKey::NodeRenamingMapKey(
    const GURL& url, const base::string16& provider_title)
    : url_(url), provider_title_(provider_title) {}

PartnerBookmarksShim::NodeRenamingMapKey::~NodeRenamingMapKey() {}

bool operator<(const PartnerBookmarksShim::NodeRenamingMapKey& a,
               const PartnerBookmarksShim::NodeRenamingMapKey& b) {
  return (a.url_ < b.url_) ||
      (a.url_ == b.url_ && a.provider_title_ < b.provider_title_);
}

// static
void PartnerBookmarksShim::ClearInBrowserContextForTesting(
    content::BrowserContext* browser_context) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  browser_context->SetUserData(kPartnerBookmarksShimUserDataKey, 0);
}

// static
void PartnerBookmarksShim::ClearPartnerModelForTesting() {
  g_partner_model_keeper.Get().loaded = false;
  g_partner_model_keeper.Get().partner_bookmarks_root.reset(0);
}

// static
void PartnerBookmarksShim::EnablePartnerBookmarksEditing() {
  g_disable_partner_bookmarks_editing = false;
}

PartnerBookmarksShim::PartnerBookmarksShim(PrefService* prefs)
    : prefs_(prefs),
      observers_(
          ObserverList<PartnerBookmarksShim::Observer>::NOTIFY_EXISTING_ONLY) {
}

PartnerBookmarksShim::~PartnerBookmarksShim() {
  FOR_EACH_OBSERVER(PartnerBookmarksShim::Observer, observers_,
                    ShimBeingDeleted(this));
}

const BookmarkNode* PartnerBookmarksShim::GetNodeByID(
    const BookmarkNode* parent, int64 id) const {
  if (parent->id() == id)
    return parent;
  for (int i = 0, child_count = parent->child_count(); i < child_count; ++i) {
    const BookmarkNode* result = GetNodeByID(parent->GetChild(i), id);
    if (result)
      return result;
  }
  return NULL;
}

void PartnerBookmarksShim::ReloadNodeMapping() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  node_rename_remove_map_.clear();
  if (!prefs_)
    return;

  const base::ListValue* list =
      prefs_->GetList(prefs::kPartnerBookmarkMappings);
  if (!list)
    return;

  for (base::ListValue::const_iterator it = list->begin();
       it != list->end(); ++it) {
    const base::DictionaryValue* dict = NULL;
    if (!*it || !(*it)->GetAsDictionary(&dict)) {
      NOTREACHED();
      continue;
    }

    std::string url;
    base::string16 provider_title;
    base::string16 mapped_title;
    if (!dict->GetString(kMappingUrl, &url) ||
        !dict->GetString(kMappingProviderTitle, &provider_title) ||
        !dict->GetString(kMappingTitle, &mapped_title)) {
      NOTREACHED();
      continue;
    }

    const NodeRenamingMapKey key(GURL(url), provider_title);
    node_rename_remove_map_[key] = mapped_title;
  }
}

void PartnerBookmarksShim::SaveNodeMapping() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  if (!prefs_)
    return;

  base::ListValue list;
  for (NodeRenamingMap::const_iterator i = node_rename_remove_map_.begin();
       i != node_rename_remove_map_.end();
       ++i) {
    base::DictionaryValue* dict = new base::DictionaryValue();
    dict->SetString(kMappingUrl, i->first.url().spec());
    dict->SetString(kMappingProviderTitle, i->first.provider_title());
    dict->SetString(kMappingTitle, i->second);
    list.Append(dict);
  }
  prefs_->Set(prefs::kPartnerBookmarkMappings, list);
}
