// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_EXTENSIONS_EXTENSION_ACTION_VIEW_CONTROLLER_H_
#define CHROME_BROWSER_UI_EXTENSIONS_EXTENSION_ACTION_VIEW_CONTROLLER_H_

#include "base/memory/weak_ptr.h"
#include "base/scoped_observer.h"
#include "chrome/browser/extensions/extension_action_icon_factory.h"
#include "chrome/browser/extensions/extension_context_menu_model.h"
#include "chrome/browser/ui/toolbar/toolbar_action_view_controller.h"
#include "extensions/browser/extension_host_observer.h"
#include "ui/gfx/image/image.h"

class Browser;
class ExtensionAction;
class ExtensionActionPlatformDelegate;
class GURL;
class IconWithBadgeImageSource;
class ToolbarActionsBar;

namespace extensions {
class Command;
class Extension;
class ExtensionRegistry;
class ExtensionViewHost;
}

// The platform-independent controller for an ExtensionAction that is shown on
// the toolbar (such as a page or browser action).
// Since this class doesn't own the extension or extension action in question,
// be sure to check for validity using ExtensionIsValid() before using those
// members (see also comments above ExtensionIsValid()).
class ExtensionActionViewController
    : public ToolbarActionViewController,
      public ExtensionActionIconFactory::Observer,
      public extensions::ExtensionContextMenuModel::PopupDelegate,
      public extensions::ExtensionHostObserver {
 public:
  // The different options for showing a popup.
  enum PopupShowAction { SHOW_POPUP, SHOW_POPUP_AND_INSPECT };

  ExtensionActionViewController(const extensions::Extension* extension,
                                Browser* browser,
                                ExtensionAction* extension_action,
                                ToolbarActionsBar* toolbar_actions_bar);
  ~ExtensionActionViewController() override;

  // ToolbarActionViewController:
  std::string GetId() const override;
  void SetDelegate(ToolbarActionViewDelegate* delegate) override;
  gfx::Image GetIcon(content::WebContents* web_contents,
                     const gfx::Size& size) override;
  base::string16 GetActionName() const override;
  base::string16 GetAccessibleName(content::WebContents* web_contents) const
      override;
  base::string16 GetTooltip(content::WebContents* web_contents) const override;
  bool IsEnabled(content::WebContents* web_contents) const override;
  bool WantsToRun(content::WebContents* web_contents) const override;
  bool HasPopup(content::WebContents* web_contents) const override;
  void HidePopup() override;
  gfx::NativeView GetPopupNativeView() override;
  ui::MenuModel* GetContextMenu() override;
  void OnContextMenuClosed() override;
  bool ExecuteAction(bool by_user) override;
  void UpdateState() override;
  void RegisterCommand() override;
  bool DisabledClickOpensMenu() const override;

  // ExtensionContextMenuModel::PopupDelegate:
  void InspectPopup() override;

  // Closes the active popup (whether it was this action's popup or not).
  void HideActivePopup();

  // Populates |command| with the command associated with |extension|, if one
  // exists. Returns true if |command| was populated.
  bool GetExtensionCommand(extensions::Command* command);

  const extensions::Extension* extension() const { return extension_.get(); }
  Browser* browser() { return browser_; }
  ExtensionAction* extension_action() { return extension_action_; }
  const ExtensionAction* extension_action() const { return extension_action_; }
  ToolbarActionViewDelegate* view_delegate() { return view_delegate_; }
  bool is_showing_popup() const { return popup_host_ != nullptr; }

  void set_icon_observer(ExtensionActionIconFactory::Observer* icon_observer) {
    icon_observer_ = icon_observer;
  }

  scoped_ptr<IconWithBadgeImageSource> GetIconImageSourceForTesting(
      content::WebContents* web_contents,
      const gfx::Size& size);

 private:
  // ExtensionActionIconFactory::Observer:
  void OnIconUpdated() override;

  // ExtensionHostObserver:
  void OnExtensionHostDestroyed(const extensions::ExtensionHost* host) override;

  // Checks if the associated |extension| is still valid by checking its
  // status in the registry. Since the OnExtensionUnloaded() notifications are
  // not in a deterministic order, it's possible that the view tries to refresh
  // itself before we're notified to remove it.
  bool ExtensionIsValid() const;

  // In some cases (such as when an action is shown in a menu), a substitute
  // ToolbarActionViewController should be used for showing popups. This
  // returns the preferred controller.
  ExtensionActionViewController* GetPreferredPopupViewController();

  // Executes the extension action with |show_action|. If
  // |grant_tab_permissions| is true, this will grant the extension active tab
  // permissions. Only do this if this was done through a user action (and not
  // e.g. an API). Returns true if a popup is shown.
  bool ExecuteAction(PopupShowAction show_action, bool grant_tab_permissions);

  // Begins the process of showing the popup for the extension action, given the
  // associated |popup_url|. |grant_tab_permissions| is true if active tab
  // permissions should be given to the extension; this is only true if the
  // popup is opened through a user action.
  // The popup may not be shown synchronously if the extension is hidden and
  // first needs to slide itself out.
  // Returns true if a popup will be shown.
  bool TriggerPopupWithUrl(PopupShowAction show_action,
                           const GURL& popup_url,
                           bool grant_tab_permissions);

  // Shows the popup with the given |host|.
  void ShowPopup(scoped_ptr<extensions::ExtensionViewHost> host,
                 bool grant_tab_permissions,
                 PopupShowAction show_action);

  // Handles cleanup after the popup closes.
  void OnPopupClosed();

  // Returns the image source for the icon.
  scoped_ptr<IconWithBadgeImageSource> GetIconImageSource(
      content::WebContents* web_contents,
      const gfx::Size& size);

  // The extension associated with the action we're displaying.
  scoped_refptr<const extensions::Extension> extension_;

  // The corresponding browser.
  Browser* browser_;

  // The browser action this view represents. The ExtensionAction is not owned
  // by this class.
  ExtensionAction* extension_action_;

  // The owning ToolbarActionsBar, if any. This will be null if this is a
  // page action without the toolbar redesign turned on.
  // TODO(devlin): Would this be better behind a delegate interface? On the one
  // hand, it's odd for this class to know about ToolbarActionsBar, but on the
  // other, yet-another-delegate-class might just confuse things.
  ToolbarActionsBar* toolbar_actions_bar_;

  // The extension popup's host if the popup is visible; null otherwise.
  extensions::ExtensionViewHost* popup_host_;

  // The context menu model for the extension.
  scoped_ptr<extensions::ExtensionContextMenuModel> context_menu_model_;

  // Our view delegate.
  ToolbarActionViewDelegate* view_delegate_;

  // The delegate to handle platform-specific implementations.
  scoped_ptr<ExtensionActionPlatformDelegate> platform_delegate_;

  // The object that will be used to get the browser action icon for us.
  // It may load the icon asynchronously (in which case the initial icon
  // returned by the factory will be transparent), so we have to observe it for
  // updates to the icon.
  ExtensionActionIconFactory icon_factory_;

  // An additional observer that we need to notify when the icon of the button
  // has been updated.
  ExtensionActionIconFactory::Observer* icon_observer_;

  // The associated ExtensionRegistry; cached for quick checking.
  extensions::ExtensionRegistry* extension_registry_;

  ScopedObserver<extensions::ExtensionHost, extensions::ExtensionHostObserver>
      popup_host_observer_;

  base::WeakPtrFactory<ExtensionActionViewController> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ExtensionActionViewController);
};

#endif  // CHROME_BROWSER_UI_EXTENSIONS_EXTENSION_ACTION_VIEW_CONTROLLER_H_
