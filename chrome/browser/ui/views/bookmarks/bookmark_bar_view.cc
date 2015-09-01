// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/bookmarks/bookmark_bar_view.h"

#include <algorithm>
#include <limits>
#include <string>
#include <vector>

#include "base/bind.h"
#include "base/i18n/rtl.h"
#include "base/location.h"
#include "base/metrics/histogram.h"
#include "base/prefs/pref_service.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/thread_task_runner_handle.h"
#include "chrome/browser/bookmarks/bookmark_model_factory.h"
#include "chrome/browser/bookmarks/managed_bookmark_service_factory.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/defaults.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search/search.h"
#include "chrome/browser/sync/profile_sync_service.h"
#include "chrome/browser/sync/profile_sync_service_factory.h"
#include "chrome/browser/themes/theme_properties.h"
#include "chrome/browser/ui/bookmarks/bookmark_bar_constants.h"
#include "chrome/browser/ui/bookmarks/bookmark_drag_drop.h"
#include "chrome/browser/ui/bookmarks/bookmark_tab_helper.h"
#include "chrome/browser/ui/bookmarks/bookmark_utils.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/chrome_pages.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/view_ids.h"
#include "chrome/browser/ui/views/bookmarks/bookmark_bar_instructions_view.h"
#include "chrome/browser/ui/views/bookmarks/bookmark_bar_view_observer.h"
#include "chrome/browser/ui/views/bookmarks/bookmark_context_menu.h"
#include "chrome/browser/ui/views/bookmarks/bookmark_menu_controller_views.h"
#include "chrome/browser/ui/views/event_utils.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/location_bar/location_bar_view.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/extensions/extension_constants.h"
#include "chrome/common/extensions/extension_metrics.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/url_constants.h"
#include "chrome/grit/generated_resources.h"
#include "components/bookmarks/browser/bookmark_model.h"
#include "components/bookmarks/browser/bookmark_utils.h"
#include "components/bookmarks/managed/managed_bookmark_service.h"
#include "components/metrics/metrics_service.h"
#include "components/omnibox/browser/omnibox_popup_model.h"
#include "components/omnibox/browser/omnibox_view.h"
#include "components/url_formatter/elide_url.h"
#include "content/public/browser/notification_details.h"
#include "content/public/browser/notification_source.h"
#include "content/public/browser/page_navigator.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/user_metrics.h"
#include "content/public/browser/web_contents.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_set.h"
#include "grit/theme_resources.h"
#include "ui/accessibility/ax_view_state.h"
#include "ui/base/dragdrop/drag_utils.h"
#include "ui/base/dragdrop/os_exchange_data.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/page_transition_types.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/theme_provider.h"
#include "ui/base/window_open_disposition.h"
#include "ui/compositor/paint_recorder.h"
#include "ui/gfx/animation/slide_animation.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/text_constants.h"
#include "ui/gfx/text_elider.h"
#include "ui/resources/grit/ui_resources.h"
#include "ui/views/button_drag_utils.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/button/label_button_border.h"
#include "ui/views/controls/button/menu_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/drag_utils.h"
#include "ui/views/metrics.h"
#include "ui/views/view_constants.h"
#include "ui/views/widget/tooltip_manager.h"
#include "ui/views/widget/widget.h"
#include "ui/views/window/non_client_view.h"

using base::UserMetricsAction;
using bookmarks::BookmarkModel;
using bookmarks::BookmarkNode;
using bookmarks::BookmarkNodeData;
using content::OpenURLParams;
using content::PageNavigator;
using content::Referrer;
using ui::DropTargetEvent;
using views::CustomButton;
using views::LabelButtonBorder;
using views::MenuButton;
using views::View;

// How inset the bookmarks bar is when displayed on the new tab page.
static const int kNewTabHorizontalPadding = 2;

// Maximum size of buttons on the bookmark bar.
static const int kMaxButtonWidth = 150;

// The color gradient start value close to the edge of the divider.
static const SkColor kEdgeDividerColor = SkColorSetRGB(222, 234, 248);

// The color gradient value for the middle of the divider.
static const SkColor kMiddleDividerColor = SkColorSetRGB(194, 205, 212);

// Number of pixels the attached bookmark bar overlaps with the toolbar.
static const int kToolbarAttachedBookmarkBarOverlap = 3;

// Margins around the content.
static const int kDetachedTopMargin = 1;  // When attached, we use 0 and let the
                                          // toolbar above serve as the margin.
static const int kBottomMargin = 2;
static const int kLeftMargin = 1;
static const int kRightMargin = 1;

// Padding between buttons.
static const int kButtonPadding = 0;

// Color of the drop indicator.
static const SkColor kDropIndicatorColor = SK_ColorBLACK;

// Width of the drop indicator.
static const int kDropIndicatorWidth = 2;

// Distance between the bottom of the bar and the separator.
static const int kSeparatorMargin = 1;

// Width of the separator between the recently bookmarked button and the
// overflow indicator.
static const int kSeparatorWidth = 4;

// Starting x-coordinate of the separator line within a separator.
static const int kSeparatorStartX = 2;

// Left-padding for the instructional text.
static const int kInstructionsPadding = 6;

// Tag for the 'Other bookmarks' button.
static const int kOtherFolderButtonTag = 1;

// Tag for the 'Apps Shortcut' button.
static const int kAppsShortcutButtonTag = 2;

// Preferred padding between text and edge.
static const int kButtonPaddingHorizontal = 6;
static const int kButtonPaddingVertical = 4;

// Tag for the 'Managed bookmarks' button.
static const int kManagedFolderButtonTag = 3;
// Tag for the 'Supervised bookmarks' button.
static const int kSupervisedFolderButtonTag = 4;

#if !defined(OS_WIN)
static const gfx::ElideBehavior kElideBehavior = gfx::FADE_TAIL;
#else
// Windows fade eliding causes text to darken; see http://crbug.com/388084
static const gfx::ElideBehavior kElideBehavior = gfx::ELIDE_TAIL;
#endif

namespace {

// To enable/disable BookmarkBar animations during testing. In production
// animations are enabled by default.
bool animations_enabled = true;

gfx::ImageSkia* GetImageSkiaNamed(int id) {
  return ui::ResourceBundle::GetSharedInstance().GetImageSkiaNamed(id);
}

// BookmarkButtonBase -----------------------------------------------

// Base class for buttons used on the bookmark bar.

class BookmarkButtonBase : public views::LabelButton {
 public:
  BookmarkButtonBase(views::ButtonListener* listener,
                     const base::string16& title)
      : LabelButton(listener, title) {
    SetElideBehavior(kElideBehavior);
    show_animation_.reset(new gfx::SlideAnimation(this));
    if (!animations_enabled) {
      // For some reason during testing the events generated by animating
      // throw off the test. So, don't animate while testing.
      show_animation_->Reset(1);
    } else {
      show_animation_->Show();
    }
  }

  View* GetTooltipHandlerForPoint(const gfx::Point& point) override {
    return HitTestPoint(point) && CanProcessEventsWithinSubtree() ? this : NULL;
  }

  scoped_ptr<LabelButtonBorder> CreateDefaultBorder() const override {
    scoped_ptr<LabelButtonBorder> border = LabelButton::CreateDefaultBorder();
    border->set_insets(gfx::Insets(kButtonPaddingVertical,
                                   kButtonPaddingHorizontal,
                                   kButtonPaddingVertical,
                                   kButtonPaddingHorizontal));
    return border.Pass();
  }

  bool IsTriggerableEvent(const ui::Event& e) override {
    return e.type() == ui::ET_GESTURE_TAP ||
           e.type() == ui::ET_GESTURE_TAP_DOWN ||
           event_utils::IsPossibleDispositionEvent(e);
  }

 private:
  scoped_ptr<gfx::SlideAnimation> show_animation_;

  DISALLOW_COPY_AND_ASSIGN(BookmarkButtonBase);
};

// BookmarkButton -------------------------------------------------------------

// Buttons used for the bookmarks on the bookmark bar.

class BookmarkButton : public BookmarkButtonBase {
 public:
  // The internal view class name.
  static const char kViewClassName[];

  BookmarkButton(views::ButtonListener* listener,
                 const GURL& url,
                 const base::string16& title,
                 Profile* profile)
      : BookmarkButtonBase(listener, title),
        url_(url),
        profile_(profile) {
  }

  bool GetTooltipText(const gfx::Point& p,
                      base::string16* tooltip) const override {
    gfx::Point location(p);
    ConvertPointToScreen(this, &location);
    *tooltip = BookmarkBarView::CreateToolTipForURLAndTitle(
        GetWidget(), location, url_, GetText(), profile_);
    return !tooltip->empty();
  }

  const char* GetClassName() const override { return kViewClassName; }

 private:
  const GURL& url_;
  Profile* profile_;

  DISALLOW_COPY_AND_ASSIGN(BookmarkButton);
};

// static
const char BookmarkButton::kViewClassName[] = "BookmarkButton";

// ShortcutButton -------------------------------------------------------------

// Buttons used for the shortcuts on the bookmark bar.

class ShortcutButton : public BookmarkButtonBase {
 public:
  // The internal view class name.
  static const char kViewClassName[];

  ShortcutButton(views::ButtonListener* listener,
                 const base::string16& title)
      : BookmarkButtonBase(listener, title) {
  }

  const char* GetClassName() const override { return kViewClassName; }

 private:
  DISALLOW_COPY_AND_ASSIGN(ShortcutButton);
};

// static
const char ShortcutButton::kViewClassName[] = "ShortcutButton";

// BookmarkFolderButton -------------------------------------------------------

// Buttons used for folders on the bookmark bar, including the 'other folders'
// button.
class BookmarkFolderButton : public views::MenuButton {
 public:
  BookmarkFolderButton(views::ButtonListener* listener,
                       const base::string16& title,
                       views::MenuButtonListener* menu_button_listener,
                       bool show_menu_marker)
      : MenuButton(listener, title, menu_button_listener, show_menu_marker) {
    SetElideBehavior(kElideBehavior);
    show_animation_.reset(new gfx::SlideAnimation(this));
    if (!animations_enabled) {
      // For some reason during testing the events generated by animating
      // throw off the test. So, don't animate while testing.
      show_animation_->Reset(1);
    } else {
      show_animation_->Show();
    }
  }

  bool GetTooltipText(const gfx::Point& p,
                      base::string16* tooltip) const override {
    if (label()->GetPreferredSize().width() > label()->size().width())
      *tooltip = GetText();
    return !tooltip->empty();
  }

  bool IsTriggerableEvent(const ui::Event& e) override {
    // Left clicks and taps should show the menu contents and right clicks
    // should show the context menu. They should not trigger the opening of
    // underlying urls.
    if (e.type() == ui::ET_GESTURE_TAP ||
        (e.IsMouseEvent() && (e.flags() &
             (ui::EF_LEFT_MOUSE_BUTTON | ui::EF_RIGHT_MOUSE_BUTTON))))
      return false;

    if (e.IsMouseEvent())
      return ui::DispositionFromEventFlags(e.flags()) != CURRENT_TAB;
    return false;
  }

 private:
  scoped_ptr<gfx::SlideAnimation> show_animation_;

  DISALLOW_COPY_AND_ASSIGN(BookmarkFolderButton);
};

// OverFlowButton (chevron) --------------------------------------------------

class OverFlowButton : public views::MenuButton {
 public:
  explicit OverFlowButton(BookmarkBarView* owner)
      : MenuButton(NULL, base::string16(), owner, false),
        owner_(owner) {}

  bool OnMousePressed(const ui::MouseEvent& e) override {
    owner_->StopThrobbing(true);
    return views::MenuButton::OnMousePressed(e);
  }

 private:
  BookmarkBarView* owner_;

  DISALLOW_COPY_AND_ASSIGN(OverFlowButton);
};

void RecordAppLaunch(Profile* profile, const GURL& url) {
  const extensions::Extension* extension =
      extensions::ExtensionRegistry::Get(profile)
          ->enabled_extensions().GetAppByURL(url);
  if (!extension)
    return;

  extensions::RecordAppLaunchType(extension_misc::APP_LAUNCH_BOOKMARK_BAR,
                                  extension->GetType());
}

}  // namespace

// DropLocation ---------------------------------------------------------------

struct BookmarkBarView::DropLocation {
  DropLocation()
      : index(-1),
        operation(ui::DragDropTypes::DRAG_NONE),
        on(false),
        button_type(DROP_BOOKMARK) {
  }

  bool Equals(const DropLocation& other) {
    return ((other.index == index) && (other.on == on) &&
            (other.button_type == button_type));
  }

  // Index into the model the drop is over. This is relative to the root node.
  int index;

  // Drop constants.
  int operation;

  // If true, the user is dropping on a folder.
  bool on;

  // Type of button.
  DropButtonType button_type;
};

// DropInfo -------------------------------------------------------------------

// Tracks drops on the BookmarkBarView.

struct BookmarkBarView::DropInfo {
  DropInfo()
      : valid(false),
        is_menu_showing(false),
        x(0),
        y(0) {
  }

  // Whether the data is valid.
  bool valid;

  // If true, the menu is being shown.
  bool is_menu_showing;

  // Coordinates of the drag (in terms of the BookmarkBarView).
  int x;
  int y;

  // DropData for the drop.
  BookmarkNodeData data;

  DropLocation location;
};

// ButtonSeparatorView  --------------------------------------------------------

// Paints a themed gradient divider at location |x|. |height| is the full
// height of the view you want to paint the divider into, not the height of
// the divider. The height of the divider will become:
//   |height| - 2 * |vertical_padding|.
// The color of the divider is a gradient starting with |top_color| at the
// top, and changing into |middle_color| and then over to |bottom_color| as
// you go further down.
void PaintVerticalDivider(gfx::Canvas* canvas,
                          int x,
                          int height,
                          int vertical_padding,
                          SkColor top_color,
                          SkColor middle_color,
                          SkColor bottom_color) {
  // Draw the upper half of the divider.
  SkPaint paint;
  skia::RefPtr<SkShader> shader = gfx::CreateGradientShader(
      vertical_padding + 1, height / 2, top_color, middle_color);
  paint.setShader(shader.get());
  SkRect rc = { SkIntToScalar(x),
                SkIntToScalar(vertical_padding + 1),
                SkIntToScalar(x + 1),
                SkIntToScalar(height / 2) };
  canvas->sk_canvas()->drawRect(rc, paint);

  // Draw the lower half of the divider.
  SkPaint paint_down;
  shader = gfx::CreateGradientShader(
      height / 2, height - vertical_padding, middle_color, bottom_color);
  paint_down.setShader(shader.get());
  SkRect rc_down = { SkIntToScalar(x),
                     SkIntToScalar(height / 2),
                     SkIntToScalar(x + 1),
                     SkIntToScalar(height - vertical_padding) };
  canvas->sk_canvas()->drawRect(rc_down, paint_down);
}

class BookmarkBarView::ButtonSeparatorView : public views::View {
 public:
  ButtonSeparatorView() {}
  ~ButtonSeparatorView() override {}

  void OnPaint(gfx::Canvas* canvas) override {
    PaintVerticalDivider(
        canvas,
        kSeparatorStartX,
        height(),
        1,
        kEdgeDividerColor,
        kMiddleDividerColor,
        GetThemeProvider()->GetColor(ThemeProperties::COLOR_TOOLBAR));
  }

  gfx::Size GetPreferredSize() const override {
    // We get the full height of the bookmark bar, so that the height returned
    // here doesn't matter.
    return gfx::Size(kSeparatorWidth, 1);
  }

  void GetAccessibleState(ui::AXViewState* state) override {
    state->name = l10n_util::GetStringUTF16(IDS_ACCNAME_SEPARATOR);
    state->role = ui::AX_ROLE_SPLITTER;
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(ButtonSeparatorView);
};

// BookmarkBarView ------------------------------------------------------------

// static
const char BookmarkBarView::kViewClassName[] = "BookmarkBarView";

BookmarkBarView::BookmarkBarView(Browser* browser, BrowserView* browser_view)
    : page_navigator_(NULL),
      managed_(NULL),
      bookmark_menu_(NULL),
      bookmark_drop_menu_(NULL),
      other_bookmarks_button_(NULL),
      managed_bookmarks_button_(NULL),
      supervised_bookmarks_button_(NULL),
      apps_page_shortcut_(NULL),
      overflow_button_(NULL),
      instructions_(NULL),
      bookmarks_separator_view_(NULL),
      browser_(browser),
      browser_view_(browser_view),
      infobar_visible_(false),
      throbbing_view_(NULL),
      bookmark_bar_state_(BookmarkBar::SHOW),
      animating_detached_(false),
      show_folder_method_factory_(this) {
  set_id(VIEW_ID_BOOKMARK_BAR);
  Init();

  size_animation_->Reset(1);
}

BookmarkBarView::~BookmarkBarView() {
  if (model_)
    model_->RemoveObserver(this);

  // It's possible for the menu to outlive us, reset the observer to make sure
  // it doesn't have a reference to us.
  if (bookmark_menu_) {
    bookmark_menu_->set_observer(NULL);
    bookmark_menu_->SetPageNavigator(NULL);
    bookmark_menu_->clear_bookmark_bar();
  }
  if (context_menu_.get())
    context_menu_->SetPageNavigator(NULL);

  StopShowFolderDropMenuTimer();
}

// static
void BookmarkBarView::DisableAnimationsForTesting(bool disabled) {
  animations_enabled = !disabled;
}

void BookmarkBarView::AddObserver(BookmarkBarViewObserver* observer) {
  observers_.AddObserver(observer);
}

void BookmarkBarView::RemoveObserver(BookmarkBarViewObserver* observer) {
  observers_.RemoveObserver(observer);
}

void BookmarkBarView::SetPageNavigator(PageNavigator* navigator) {
  page_navigator_ = navigator;
  if (bookmark_menu_)
    bookmark_menu_->SetPageNavigator(navigator);
  if (context_menu_.get())
    context_menu_->SetPageNavigator(navigator);
}

void BookmarkBarView::SetBookmarkBarState(
    BookmarkBar::State state,
    BookmarkBar::AnimateChangeType animate_type) {
  if (animate_type == BookmarkBar::ANIMATE_STATE_CHANGE &&
      animations_enabled) {
    animating_detached_ = (state == BookmarkBar::DETACHED ||
                           bookmark_bar_state_ == BookmarkBar::DETACHED);
    if (state == BookmarkBar::SHOW)
      size_animation_->Show();
    else
      size_animation_->Hide();
  } else {
    size_animation_->Reset(state == BookmarkBar::SHOW ? 1 : 0);
  }
  bookmark_bar_state_ = state;
}

int BookmarkBarView::GetFullyDetachedToolbarOverlap() const {
  if (!infobar_visible_ && browser_->window()->IsFullscreen()) {
    // There is no client edge to overlap when detached in fullscreen with no
    // infobars visible.
    return 0;
  }
  return views::NonClientFrameView::kClientEdgeThickness;
}

bool BookmarkBarView::is_animating() {
  return size_animation_->is_animating();
}

const BookmarkNode* BookmarkBarView::GetNodeForButtonAtModelIndex(
    const gfx::Point& loc,
    int* model_start_index) {
  *model_start_index = 0;

  if (loc.x() < 0 || loc.x() >= width() || loc.y() < 0 || loc.y() >= height())
    return NULL;

  gfx::Point adjusted_loc(GetMirroredXInView(loc.x()), loc.y());

  // Check the managed button first.
  if (managed_bookmarks_button_->visible() &&
      managed_bookmarks_button_->bounds().Contains(adjusted_loc)) {
    return managed_->managed_node();
  }

  // Then check the supervised button.
  if (supervised_bookmarks_button_->visible() &&
      supervised_bookmarks_button_->bounds().Contains(adjusted_loc)) {
    return managed_->supervised_node();
  }

  // Then check the bookmark buttons.
  for (int i = 0; i < GetBookmarkButtonCount(); ++i) {
    views::View* child = child_at(i);
    if (!child->visible())
      break;
    if (child->bounds().Contains(adjusted_loc))
      return model_->bookmark_bar_node()->GetChild(i);
  }

  // Then the overflow button.
  if (overflow_button_->visible() &&
      overflow_button_->bounds().Contains(adjusted_loc)) {
    *model_start_index = GetFirstHiddenNodeIndex();
    return model_->bookmark_bar_node();
  }

  // And finally the other folder.
  if (other_bookmarks_button_->visible() &&
      other_bookmarks_button_->bounds().Contains(adjusted_loc)) {
    return model_->other_node();
  }

  return NULL;
}

views::MenuButton* BookmarkBarView::GetMenuButtonForNode(
    const BookmarkNode* node) {
  if (node == managed_->managed_node())
    return managed_bookmarks_button_;
  if (node == managed_->supervised_node())
    return supervised_bookmarks_button_;
  if (node == model_->other_node())
    return other_bookmarks_button_;
  if (node == model_->bookmark_bar_node())
    return overflow_button_;
  int index = model_->bookmark_bar_node()->GetIndexOf(node);
  if (index == -1 || !node->is_folder())
    return NULL;
  return static_cast<views::MenuButton*>(child_at(index));
}

void BookmarkBarView::GetAnchorPositionForButton(
    views::MenuButton* button,
    views::MenuAnchorPosition* anchor) {
  if (button == other_bookmarks_button_ || button == overflow_button_)
    *anchor = views::MENU_ANCHOR_TOPRIGHT;
  else
    *anchor = views::MENU_ANCHOR_TOPLEFT;
}

views::MenuItemView* BookmarkBarView::GetMenu() {
  return bookmark_menu_ ? bookmark_menu_->menu() : NULL;
}

views::MenuItemView* BookmarkBarView::GetContextMenu() {
  return bookmark_menu_ ? bookmark_menu_->context_menu() : NULL;
}

views::MenuItemView* BookmarkBarView::GetDropMenu() {
  return bookmark_drop_menu_ ? bookmark_drop_menu_->menu() : NULL;
}

void BookmarkBarView::StopThrobbing(bool immediate) {
  if (!throbbing_view_)
    return;

  // If not immediate, cycle through 2 more complete cycles.
  throbbing_view_->StartThrobbing(immediate ? 0 : 4);
  throbbing_view_ = NULL;
}

// static
base::string16 BookmarkBarView::CreateToolTipForURLAndTitle(
    const views::Widget* widget,
    const gfx::Point& screen_loc,
    const GURL& url,
    const base::string16& title,
    Profile* profile) {
  const views::TooltipManager* tooltip_manager = widget->GetTooltipManager();
  int max_width = tooltip_manager->GetMaxWidth(screen_loc,
                                               widget->GetNativeView());
  const gfx::FontList tt_fonts = tooltip_manager->GetFontList();
  base::string16 result;

  // First the title.
  if (!title.empty()) {
    base::string16 localized_title = title;
    base::i18n::AdjustStringForLocaleDirection(&localized_title);
    result.append(gfx::ElideText(localized_title, tt_fonts, max_width,
                                 gfx::ELIDE_TAIL));
  }

  // Only show the URL if the url and title differ.
  if (title != base::UTF8ToUTF16(url.spec())) {
    if (!result.empty())
      result.push_back('\n');

    // We need to explicitly specify the directionality of the URL's text to
    // make sure it is treated as an LTR string when the context is RTL. For
    // example, the URL "http://www.yahoo.com/" appears as
    // "/http://www.yahoo.com" when rendered, as is, in an RTL context since
    // the Unicode BiDi algorithm puts certain characters on the left by
    // default.
    std::string languages = profile->GetPrefs()->GetString(
        prefs::kAcceptLanguages);
    base::string16 elided_url(
        url_formatter::ElideUrl(url, tt_fonts, max_width, languages));
    elided_url = base::i18n::GetDisplayStringInLTRDirectionality(elided_url);
    result.append(elided_url);
  }
  return result;
}

bool BookmarkBarView::IsDetached() const {
  return (bookmark_bar_state_ == BookmarkBar::DETACHED) ||
      (animating_detached_ && size_animation_->is_animating());
}

double BookmarkBarView::GetAnimationValue() const {
  return size_animation_->GetCurrentValue();
}

int BookmarkBarView::GetToolbarOverlap() const {
  int attached_overlap = kToolbarAttachedBookmarkBarOverlap +
      views::NonClientFrameView::kClientEdgeThickness;
  if (!IsDetached())
    return attached_overlap;

  int detached_overlap = GetFullyDetachedToolbarOverlap();

  // Do not animate the overlap when the infobar is above us (i.e. when we're
  // detached), since drawing over the infobar looks weird.
  if (infobar_visible_)
    return detached_overlap;

  // When detached with no infobar, animate the overlap between the attached and
  // detached states.
  return detached_overlap + static_cast<int>(
      (attached_overlap - detached_overlap) *
          size_animation_->GetCurrentValue());
}

gfx::Size BookmarkBarView::GetPreferredSize() const {
  gfx::Size prefsize;
  if (IsDetached()) {
    prefsize.set_height(
        chrome::kBookmarkBarHeight +
        static_cast<int>(
            (chrome::kNTPBookmarkBarHeight - chrome::kBookmarkBarHeight) *
            (1 - size_animation_->GetCurrentValue())));
  } else {
    prefsize.set_height(static_cast<int>(chrome::kBookmarkBarHeight *
                                         size_animation_->GetCurrentValue()));
  }
  return prefsize;
}

bool BookmarkBarView::CanProcessEventsWithinSubtree() const {
  // If the bookmark bar is attached and the omnibox popup is open (on top of
  // the bar), prevent events from targeting the bookmark bar or any of its
  // descendants. This will prevent hovers/clicks just above the omnibox popup
  // from activating the top few pixels of items on the bookmark bar.
  if (!IsDetached() && browser_view_ &&
      browser_view_->GetLocationBar()->GetOmniboxView()->model()->
          popup_model()->IsOpen()) {
    return false;
  }
  return true;
}

gfx::Size BookmarkBarView::GetMinimumSize() const {
  // The minimum width of the bookmark bar should at least contain the overflow
  // button, by which one can access all the Bookmark Bar items, and the "Other
  // Bookmarks" folder, along with appropriate margins and button padding.
  // It should also contain the Managed and/or Supervised Bookmarks folders,
  // if they are visible.
  int width = kLeftMargin;

  int height = chrome::kBookmarkBarHeight;
  if (IsDetached()) {
    double current_state = 1 - size_animation_->GetCurrentValue();
    width += 2 * static_cast<int>(kNewTabHorizontalPadding * current_state);
    height += static_cast<int>(
        (chrome::kNTPBookmarkBarHeight - chrome::kBookmarkBarHeight) *
            current_state);
  }

  if (managed_bookmarks_button_->visible()) {
    gfx::Size size = managed_bookmarks_button_->GetPreferredSize();
    width += size.width() + kButtonPadding;
  }
  if (supervised_bookmarks_button_->visible()) {
    gfx::Size size = supervised_bookmarks_button_->GetPreferredSize();
    width += size.width() + kButtonPadding;
  }
  if (other_bookmarks_button_->visible()) {
    gfx::Size size = other_bookmarks_button_->GetPreferredSize();
    width += size.width() + kButtonPadding;
  }
  if (overflow_button_->visible()) {
    gfx::Size size = overflow_button_->GetPreferredSize();
    width += size.width() + kButtonPadding;
  }
  if (bookmarks_separator_view_->visible()) {
    gfx::Size size = bookmarks_separator_view_->GetPreferredSize();
    width += size.width();
  }
  if (apps_page_shortcut_->visible()) {
    gfx::Size size = apps_page_shortcut_->GetPreferredSize();
    width += size.width() + kButtonPadding;
  }

  return gfx::Size(width, height);
}

void BookmarkBarView::Layout() {
  // Skip layout during destruction, when no model exists.
  if (!model_)
    return;

  int x = kLeftMargin;
  int top_margin = IsDetached() ? kDetachedTopMargin : 0;
  int y = top_margin;
  int width = View::width() - kRightMargin - kLeftMargin;
  int height = chrome::kBookmarkBarHeight - kBottomMargin;
  int separator_margin = kSeparatorMargin;

  if (IsDetached()) {
    double current_state = 1 - size_animation_->GetCurrentValue();
    x += static_cast<int>(kNewTabHorizontalPadding * current_state);
    y += (View::height() - chrome::kBookmarkBarHeight) / 2;
    width -= static_cast<int>(kNewTabHorizontalPadding * current_state);
    separator_margin -= static_cast<int>(kSeparatorMargin * current_state);
  } else {
    // For the attached appearance, pin the content to the bottom of the bar
    // when animating in/out, as shrinking its height instead looks weird.  This
    // also matches how we layout infobars.
    y += View::height() - chrome::kBookmarkBarHeight;
  }

  gfx::Size other_bookmarks_pref = other_bookmarks_button_->visible() ?
      other_bookmarks_button_->GetPreferredSize() : gfx::Size();
  gfx::Size overflow_pref = overflow_button_->GetPreferredSize();
  gfx::Size bookmarks_separator_pref =
      bookmarks_separator_view_->GetPreferredSize();
  gfx::Size apps_page_shortcut_pref = apps_page_shortcut_->visible() ?
      apps_page_shortcut_->GetPreferredSize() : gfx::Size();

  int max_x = width - overflow_pref.width() - kButtonPadding -
      bookmarks_separator_pref.width();
  if (other_bookmarks_button_->visible())
    max_x -= other_bookmarks_pref.width() + kButtonPadding;

  // Start with the apps page shortcut button.
  if (apps_page_shortcut_->visible()) {
    apps_page_shortcut_->SetBounds(x, y, apps_page_shortcut_pref.width(),
                                   height);
    x += apps_page_shortcut_pref.width() + kButtonPadding;
  }

  // Then comes the managed bookmarks folder, if visible.
  if (managed_bookmarks_button_->visible()) {
    gfx::Size managed_bookmarks_pref =
        managed_bookmarks_button_->GetPreferredSize();
    managed_bookmarks_button_->SetBounds(x, y, managed_bookmarks_pref.width(),
                                         height);
    x += managed_bookmarks_pref.width() + kButtonPadding;
  }

  // Then the supervised bookmarks folder, if visible.
  if (supervised_bookmarks_button_->visible()) {
    gfx::Size supervised_bookmarks_pref =
        supervised_bookmarks_button_->GetPreferredSize();
    supervised_bookmarks_button_->SetBounds(
        x, y, supervised_bookmarks_pref.width(), height);
    x += supervised_bookmarks_pref.width() + kButtonPadding;
  }

  const bool show_instructions =
      model_ && model_->loaded() &&
      model_->bookmark_bar_node()->child_count() == 0;
  instructions_->SetVisible(show_instructions);
  if (show_instructions) {
    gfx::Size pref = instructions_->GetPreferredSize();
    instructions_->SetBounds(
        x + kInstructionsPadding, y,
        std::min(static_cast<int>(pref.width()),
                 max_x - x),
        height);
  } else {
    bool last_visible = x < max_x;
    int button_count = GetBookmarkButtonCount();
    for (int i = 0; i <= button_count; ++i) {
      if (i == button_count) {
        // Add another button if there is room for it (and there is another
        // button to load).
        if (!last_visible || !model_->loaded() ||
            model_->bookmark_bar_node()->child_count() <= button_count)
          break;
        AddChildViewAt(
            CreateBookmarkButton(model_->bookmark_bar_node()->GetChild(i)), i);
        button_count = GetBookmarkButtonCount();
      }
      views::View* child = child_at(i);
      gfx::Size pref = child->GetPreferredSize();
      int next_x = x + pref.width() + kButtonPadding;
      last_visible = next_x < max_x;
      child->SetVisible(last_visible);
      // Only need to set bounds if the view is actually visible.
      if (last_visible)
        child->SetBounds(x, y, pref.width(), height);
      x = next_x;
    }
  }

  // Layout the right side buttons.
  x = max_x + kButtonPadding;

  // The overflow button.
  overflow_button_->SetBounds(x, y, overflow_pref.width(), height);
  const bool show_overflow =
      model_->loaded() &&
      (model_->bookmark_bar_node()->child_count() > GetBookmarkButtonCount() ||
       (GetBookmarkButtonCount() > 0 &&
        !GetBookmarkButton(GetBookmarkButtonCount() - 1)->visible()));
  overflow_button_->SetVisible(show_overflow);
  x += overflow_pref.width();

  // Separator.
  if (bookmarks_separator_view_->visible()) {
    bookmarks_separator_view_->SetBounds(x,
                                         y - top_margin,
                                         bookmarks_separator_pref.width(),
                                         height + top_margin + kBottomMargin -
                                         separator_margin);

    x += bookmarks_separator_pref.width();
  }

  // The "Other Bookmarks" button.
  if (other_bookmarks_button_->visible()) {
    other_bookmarks_button_->SetBounds(x, y, other_bookmarks_pref.width(),
                                       height);
    x += other_bookmarks_pref.width() + kButtonPadding;
  }
}

void BookmarkBarView::ViewHierarchyChanged(
    const ViewHierarchyChangedDetails& details) {
  if (details.is_add && details.child == this) {
    // We may get inserted into a hierarchy with a profile - this typically
    // occurs when the bar's contents get populated fast enough that the
    // buttons are created before the bar is attached to a frame.
    UpdateColors();

    if (height() > 0) {
      // We only layout while parented. When we become parented, if our bounds
      // haven't changed, OnBoundsChanged() won't get invoked and we won't
      // layout. Therefore we always force a layout when added.
      Layout();
    }
  }
}

void BookmarkBarView::PaintChildren(const ui::PaintContext& context) {
  View::PaintChildren(context);

  if (drop_info_.get() && drop_info_->valid &&
      drop_info_->location.operation != 0 && drop_info_->location.index != -1 &&
      drop_info_->location.button_type != DROP_OVERFLOW &&
      !drop_info_->location.on) {
    int index = drop_info_->location.index;
    DCHECK(index <= GetBookmarkButtonCount());
    int x = 0;
    int y = 0;
    int h = height();
    if (index == GetBookmarkButtonCount()) {
      if (index == 0) {
        x = kLeftMargin;
      } else {
        x = GetBookmarkButton(index - 1)->x() +
            GetBookmarkButton(index - 1)->width();
      }
    } else {
      x = GetBookmarkButton(index)->x();
    }
    if (GetBookmarkButtonCount() > 0 && GetBookmarkButton(0)->visible()) {
      y = GetBookmarkButton(0)->y();
      h = GetBookmarkButton(0)->height();
    }

    // Since the drop indicator is painted directly onto the canvas, we must
    // make sure it is painted in the right location if the locale is RTL.
    gfx::Rect indicator_bounds(x - kDropIndicatorWidth / 2,
                               y,
                               kDropIndicatorWidth,
                               h);
    indicator_bounds.set_x(GetMirroredXForRect(indicator_bounds));

    ui::PaintRecorder recorder(context, size());
    // TODO(sky/glen): make me pretty!
    recorder.canvas()->FillRect(indicator_bounds, kDropIndicatorColor);
  }
}

bool BookmarkBarView::GetDropFormats(
      int* formats,
      std::set<ui::OSExchangeData::CustomFormat>* custom_formats) {
  if (!model_ || !model_->loaded())
    return false;
  *formats = ui::OSExchangeData::URL;
  custom_formats->insert(BookmarkNodeData::GetBookmarkFormatType());
  return true;
}

bool BookmarkBarView::AreDropTypesRequired() {
  return true;
}

bool BookmarkBarView::CanDrop(const ui::OSExchangeData& data) {
  if (!model_ || !model_->loaded() ||
      !browser_->profile()->GetPrefs()->GetBoolean(
          bookmarks::prefs::kEditBookmarksEnabled))
    return false;

  if (!drop_info_.get())
    drop_info_.reset(new DropInfo());

  // Only accept drops of 1 node, which is the case for all data dragged from
  // bookmark bar and menus.
  return drop_info_->data.Read(data) && drop_info_->data.size() == 1;
}

void BookmarkBarView::OnDragEntered(const DropTargetEvent& event) {
}

int BookmarkBarView::OnDragUpdated(const DropTargetEvent& event) {
  if (!drop_info_.get())
    return 0;

  if (drop_info_->valid &&
      (drop_info_->x == event.x() && drop_info_->y == event.y())) {
    // The location of the mouse didn't change, return the last operation.
    return drop_info_->location.operation;
  }

  drop_info_->x = event.x();
  drop_info_->y = event.y();

  DropLocation location;
  CalculateDropLocation(event, drop_info_->data, &location);

  if (drop_info_->valid && drop_info_->location.Equals(location)) {
    // The position we're going to drop didn't change, return the last drag
    // operation we calculated. Copy of the operation in case it changed.
    drop_info_->location.operation = location.operation;
    return drop_info_->location.operation;
  }

  StopShowFolderDropMenuTimer();

  // TODO(sky): Optimize paint region.
  SchedulePaint();

  drop_info_->location = location;
  drop_info_->valid = true;

  if (drop_info_->is_menu_showing) {
    if (bookmark_drop_menu_)
      bookmark_drop_menu_->Cancel();
    drop_info_->is_menu_showing = false;
  }

  if (location.on || location.button_type == DROP_OVERFLOW ||
      location.button_type == DROP_OTHER_FOLDER) {
    const BookmarkNode* node;
    if (location.button_type == DROP_OTHER_FOLDER)
      node = model_->other_node();
    else if (location.button_type == DROP_OVERFLOW)
      node = model_->bookmark_bar_node();
    else
      node = model_->bookmark_bar_node()->GetChild(location.index);
    StartShowFolderDropMenuTimer(node);
  }

  return drop_info_->location.operation;
}

void BookmarkBarView::OnDragExited() {
  StopShowFolderDropMenuTimer();

  // NOTE: we don't hide the menu on exit as it's possible the user moved the
  // mouse over the menu, which triggers an exit on us.

  drop_info_->valid = false;

  if (drop_info_->location.index != -1) {
    // TODO(sky): optimize the paint region.
    SchedulePaint();
  }
  drop_info_.reset();
}

int BookmarkBarView::OnPerformDrop(const DropTargetEvent& event) {
  StopShowFolderDropMenuTimer();

  if (bookmark_drop_menu_)
    bookmark_drop_menu_->Cancel();

  if (!drop_info_.get() || !drop_info_->location.operation)
    return ui::DragDropTypes::DRAG_NONE;

  const BookmarkNode* root =
      (drop_info_->location.button_type == DROP_OTHER_FOLDER) ?
      model_->other_node() : model_->bookmark_bar_node();
  int index = drop_info_->location.index;

  if (index != -1) {
    // TODO(sky): optimize the SchedulePaint region.
    SchedulePaint();
  }
  const BookmarkNode* parent_node;
  if (drop_info_->location.button_type == DROP_OTHER_FOLDER) {
    parent_node = root;
    index = parent_node->child_count();
  } else if (drop_info_->location.on) {
    parent_node = root->GetChild(index);
    index = parent_node->child_count();
  } else {
    parent_node = root;
  }
  const BookmarkNodeData data = drop_info_->data;
  DCHECK(data.is_valid());
  bool copy = drop_info_->location.operation == ui::DragDropTypes::DRAG_COPY;
  drop_info_.reset();
  return chrome::DropBookmarks(
      browser_->profile(), data, parent_node, index, copy);
}

void BookmarkBarView::OnThemeChanged() {
  UpdateColors();
}

const char* BookmarkBarView::GetClassName() const {
  return kViewClassName;
}

void BookmarkBarView::SetVisible(bool v) {
  if (v == visible())
    return;

  View::SetVisible(v);
  FOR_EACH_OBSERVER(BookmarkBarViewObserver, observers_,
                    OnBookmarkBarVisibilityChanged());
}

void BookmarkBarView::GetAccessibleState(ui::AXViewState* state) {
  state->role = ui::AX_ROLE_TOOLBAR;
  state->name = l10n_util::GetStringUTF16(IDS_ACCNAME_BOOKMARKS);
}

void BookmarkBarView::AnimationProgressed(const gfx::Animation* animation) {
  // |browser_view_| can be NULL during tests.
  if (browser_view_)
    browser_view_->ToolbarSizeChanged(true);
}

void BookmarkBarView::AnimationEnded(const gfx::Animation* animation) {
  // |browser_view_| can be NULL during tests.
  if (browser_view_) {
    browser_view_->ToolbarSizeChanged(false);
    SchedulePaint();
  }
}

void BookmarkBarView::BookmarkMenuControllerDeleted(
    BookmarkMenuController* controller) {
  if (controller == bookmark_menu_)
    bookmark_menu_ = NULL;
  else if (controller == bookmark_drop_menu_)
    bookmark_drop_menu_ = NULL;
}

void BookmarkBarView::OnImportBookmarks() {
  int64 install_time = g_browser_process->metrics_service()->GetInstallDate();
  int64 time_from_install = base::Time::Now().ToTimeT() - install_time;
  if (bookmark_bar_state_ == BookmarkBar::SHOW) {
    UMA_HISTOGRAM_COUNTS("Import.ShowDialog.FromBookmarkBarView",
                         time_from_install);
  } else if (bookmark_bar_state_ == BookmarkBar::DETACHED) {
    UMA_HISTOGRAM_COUNTS("Import.ShowDialog.FromFloatingBookmarkBarView",
                         time_from_install);
  }

  chrome::ShowImportDialog(browser_);
}

void BookmarkBarView::OnBookmarkBubbleShown(const BookmarkNode* node) {
  StopThrobbing(true);
  if (!node)
    return;  // Generally shouldn't happen.
  StartThrobbing(node, false);
}

void BookmarkBarView::OnBookmarkBubbleHidden() {
  StopThrobbing(false);
}

void BookmarkBarView::BookmarkModelLoaded(BookmarkModel* model,
                                          bool ids_reassigned) {
  // There should be no buttons. If non-zero it means Load was invoked more than
  // once, or we didn't properly clear things. Either of which shouldn't happen.
  // The actual bookmark buttons are added from Layout().
  DCHECK_EQ(0, GetBookmarkButtonCount());
  DCHECK(model->other_node());
  other_bookmarks_button_->SetAccessibleName(model->other_node()->GetTitle());
  other_bookmarks_button_->SetText(model->other_node()->GetTitle());
  managed_bookmarks_button_->SetAccessibleName(
      managed_->managed_node()->GetTitle());
  managed_bookmarks_button_->SetText(managed_->managed_node()->GetTitle());
  supervised_bookmarks_button_->SetAccessibleName(
      managed_->supervised_node()->GetTitle());
  supervised_bookmarks_button_->SetText(
      managed_->supervised_node()->GetTitle());
  UpdateColors();
  UpdateOtherAndManagedButtonsVisibility();
  other_bookmarks_button_->SetEnabled(true);
  managed_bookmarks_button_->SetEnabled(true);
  supervised_bookmarks_button_->SetEnabled(true);
  LayoutAndPaint();
}

void BookmarkBarView::BookmarkModelBeingDeleted(BookmarkModel* model) {
  NOTREACHED();
  // Do minimal cleanup, presumably we'll be deleted shortly.
  model_->RemoveObserver(this);
  model_ = NULL;
}

void BookmarkBarView::BookmarkNodeMoved(BookmarkModel* model,
                                        const BookmarkNode* old_parent,
                                        int old_index,
                                        const BookmarkNode* new_parent,
                                        int new_index) {
  bool was_throbbing = throbbing_view_ &&
      throbbing_view_ == DetermineViewToThrobFromRemove(old_parent, old_index);
  if (was_throbbing)
    throbbing_view_->StopThrobbing();
  bool needs_layout_and_paint =
      BookmarkNodeRemovedImpl(model, old_parent, old_index);
  if (BookmarkNodeAddedImpl(model, new_parent, new_index))
    needs_layout_and_paint = true;
  if (was_throbbing && new_index < GetBookmarkButtonCount())
    StartThrobbing(new_parent->GetChild(new_index), false);
  if (needs_layout_and_paint)
    LayoutAndPaint();
}

void BookmarkBarView::BookmarkNodeAdded(BookmarkModel* model,
                                        const BookmarkNode* parent,
                                        int index) {
  if (BookmarkNodeAddedImpl(model, parent, index))
    LayoutAndPaint();
}

void BookmarkBarView::BookmarkNodeRemoved(BookmarkModel* model,
                                          const BookmarkNode* parent,
                                          int old_index,
                                          const BookmarkNode* node,
                                          const std::set<GURL>& removed_urls) {
  // Close the menu if the menu is showing for the deleted node.
  if (bookmark_menu_ && bookmark_menu_->node() == node)
    bookmark_menu_->Cancel();
  if (BookmarkNodeRemovedImpl(model, parent, old_index))
    LayoutAndPaint();
}

void BookmarkBarView::BookmarkAllUserNodesRemoved(
    BookmarkModel* model,
    const std::set<GURL>& removed_urls) {
  UpdateOtherAndManagedButtonsVisibility();

  StopThrobbing(true);

  // Remove the existing buttons.
  while (GetBookmarkButtonCount())
    delete GetBookmarkButton(0);

  LayoutAndPaint();
}

void BookmarkBarView::BookmarkNodeChanged(BookmarkModel* model,
                                          const BookmarkNode* node) {
  BookmarkNodeChangedImpl(model, node);
}

void BookmarkBarView::BookmarkNodeChildrenReordered(BookmarkModel* model,
                                                    const BookmarkNode* node) {
  if (node != model->bookmark_bar_node())
    return;  // We only care about reordering of the bookmark bar node.

  // Remove the existing buttons.
  while (GetBookmarkButtonCount())
    delete child_at(0);

  // Create the new buttons.
  for (int i = 0, child_count = node->child_count(); i < child_count; ++i)
    AddChildViewAt(CreateBookmarkButton(node->GetChild(i)), i);

  LayoutAndPaint();
}

void BookmarkBarView::BookmarkNodeFaviconChanged(BookmarkModel* model,
                                                 const BookmarkNode* node) {
  BookmarkNodeChangedImpl(model, node);
}

void BookmarkBarView::WriteDragDataForView(View* sender,
                                           const gfx::Point& press_pt,
                                           ui::OSExchangeData* data) {
  content::RecordAction(UserMetricsAction("BookmarkBar_DragButton"));

  for (int i = 0; i < GetBookmarkButtonCount(); ++i) {
    if (sender == GetBookmarkButton(i)) {
      const BookmarkNode* node = model_->bookmark_bar_node()->GetChild(i);
      const gfx::ImageSkia* icon = nullptr;
      if (node->is_url()) {
        const gfx::Image& image = model_->GetFavicon(node);
        icon = image.IsEmpty() ? GetImageSkiaNamed(IDR_DEFAULT_FAVICON)
                               : image.ToImageSkia();
      } else {
        icon = GetImageSkiaNamed(IDR_BOOKMARK_BAR_FOLDER);
      }

      button_drag_utils::SetDragImage(node->url(),
                                      node->GetTitle(),
                                      *icon,
                                      &press_pt,
                                      data,
                                      GetBookmarkButton(i)->GetWidget());
      WriteBookmarkDragData(node, data);
      return;
    }
  }
  NOTREACHED();
}

int BookmarkBarView::GetDragOperationsForView(View* sender,
                                              const gfx::Point& p) {
  if (size_animation_->is_animating() ||
      (size_animation_->GetCurrentValue() == 0 &&
       bookmark_bar_state_ != BookmarkBar::DETACHED)) {
    // Don't let the user drag while animating open or we're closed (and not
    // detached, when detached size_animation_ is always 0). This typically is
    // only hit if the user does something to inadvertently trigger DnD such as
    // pressing the mouse and hitting control-b.
    return ui::DragDropTypes::DRAG_NONE;
  }

  for (int i = 0; i < GetBookmarkButtonCount(); ++i) {
    if (sender == GetBookmarkButton(i)) {
      return chrome::GetBookmarkDragOperation(
          browser_->profile(), model_->bookmark_bar_node()->GetChild(i));
    }
  }
  NOTREACHED();
  return ui::DragDropTypes::DRAG_NONE;
}

bool BookmarkBarView::CanStartDragForView(views::View* sender,
                                          const gfx::Point& press_pt,
                                          const gfx::Point& p) {
  // Check if we have not moved enough horizontally but we have moved downward
  // vertically - downward drag.
  gfx::Vector2d move_offset = p - press_pt;
  gfx::Vector2d horizontal_offset(move_offset.x(), 0);
  if (!View::ExceededDragThreshold(horizontal_offset) && move_offset.y() > 0) {
    for (int i = 0; i < GetBookmarkButtonCount(); ++i) {
      if (sender == GetBookmarkButton(i)) {
        const BookmarkNode* node = model_->bookmark_bar_node()->GetChild(i);
        // If the folder button was dragged, show the menu instead.
        if (node && node->is_folder()) {
          views::MenuButton* menu_button =
              static_cast<views::MenuButton*>(sender);
          menu_button->Activate();
          return false;
        }
        break;
      }
    }
  }
  return true;
}

void BookmarkBarView::OnMenuButtonClicked(views::View* view,
                                          const gfx::Point& point) {
  const BookmarkNode* node;

  int start_index = 0;
  if (view == other_bookmarks_button_) {
    node = model_->other_node();
  } else if (view == managed_bookmarks_button_) {
    node = managed_->managed_node();
  } else if (view == supervised_bookmarks_button_) {
    node = managed_->supervised_node();
  } else if (view == overflow_button_) {
    node = model_->bookmark_bar_node();
    start_index = GetFirstHiddenNodeIndex();
  } else {
    int button_index = GetIndexOf(view);
    DCHECK_NE(-1, button_index);
    node = model_->bookmark_bar_node()->GetChild(button_index);
  }

  RecordBookmarkFolderOpen(GetBookmarkLaunchLocation());
  bookmark_menu_ = new BookmarkMenuController(
      browser_, page_navigator_, GetWidget(), node, start_index, false);
  bookmark_menu_->set_observer(this);
  bookmark_menu_->RunMenuAt(this);
}

void BookmarkBarView::ButtonPressed(views::Button* sender,
                                    const ui::Event& event) {
  WindowOpenDisposition disposition_from_event_flags =
      ui::DispositionFromEventFlags(event.flags());

  if (sender->tag() == kAppsShortcutButtonTag) {
    OpenURLParams params(GURL(chrome::kChromeUIAppsURL),
                         Referrer(),
                         disposition_from_event_flags,
                         ui::PAGE_TRANSITION_AUTO_BOOKMARK,
                         false);
    page_navigator_->OpenURL(params);
    RecordBookmarkAppsPageOpen(GetBookmarkLaunchLocation());
    return;
  }

  const BookmarkNode* node;
  if (sender->tag() == kOtherFolderButtonTag) {
    node = model_->other_node();
  } else if (sender->tag() == kManagedFolderButtonTag) {
    node = managed_->managed_node();
  } else if (sender->tag() == kSupervisedFolderButtonTag) {
    node = managed_->supervised_node();
  } else {
    int index = GetIndexOf(sender);
    DCHECK_NE(-1, index);
    node = model_->bookmark_bar_node()->GetChild(index);
  }
  DCHECK(page_navigator_);

  if (node->is_url()) {
    RecordAppLaunch(browser_->profile(), node->url());
    OpenURLParams params(
        node->url(), Referrer(), disposition_from_event_flags,
        ui::PAGE_TRANSITION_AUTO_BOOKMARK, false);
    page_navigator_->OpenURL(params);
  } else {
    chrome::OpenAll(GetWidget()->GetNativeWindow(), page_navigator_, node,
                    disposition_from_event_flags, browser_->profile());
  }

  RecordBookmarkLaunch(node, GetBookmarkLaunchLocation());
}

void BookmarkBarView::ShowContextMenuForView(views::View* source,
                                             const gfx::Point& point,
                                             ui::MenuSourceType source_type) {
  if (!model_->loaded()) {
    // Don't do anything if the model isn't loaded.
    return;
  }

  const BookmarkNode* parent = NULL;
  std::vector<const BookmarkNode*> nodes;
  if (source == other_bookmarks_button_) {
    parent = model_->other_node();
    // Do this so the user can open all bookmarks. BookmarkContextMenu makes
    // sure the user can't edit/delete the node in this case.
    nodes.push_back(parent);
  } else if (source == managed_bookmarks_button_) {
    parent = managed_->managed_node();
    nodes.push_back(parent);
  } else if (source == supervised_bookmarks_button_) {
    parent = managed_->supervised_node();
    nodes.push_back(parent);
  } else if (source != this && source != apps_page_shortcut_) {
    // User clicked on one of the bookmark buttons, find which one they
    // clicked on, except for the apps page shortcut, which must behave as if
    // the user clicked on the bookmark bar background.
    int bookmark_button_index = GetIndexOf(source);
    DCHECK(bookmark_button_index != -1 &&
           bookmark_button_index < GetBookmarkButtonCount());
    const BookmarkNode* node =
        model_->bookmark_bar_node()->GetChild(bookmark_button_index);
    nodes.push_back(node);
    parent = node->parent();
  } else {
    parent = model_->bookmark_bar_node();
    nodes.push_back(parent);
  }
  bool close_on_remove =
      (parent == model_->other_node()) && (parent->child_count() == 1);

  context_menu_.reset(new BookmarkContextMenu(
      GetWidget(), browser_, browser_->profile(),
      browser_->tab_strip_model()->GetActiveWebContents(),
      parent, nodes, close_on_remove));
  context_menu_->RunMenuAt(point, source_type);
}

void BookmarkBarView::Init() {
  // Note that at this point we're not in a hierarchy so GetThemeProvider() will
  // return NULL.  When we're inserted into a hierarchy, we'll call
  // UpdateColors(), which will set the appropriate colors for all the objects
  // added in this function.

  // Child views are traversed in the order they are added. Make sure the order
  // they are added matches the visual order.
  overflow_button_ = CreateOverflowButton();
  AddChildView(overflow_button_);

  other_bookmarks_button_ = CreateOtherBookmarksButton();
  // We'll re-enable when the model is loaded.
  other_bookmarks_button_->SetEnabled(false);
  AddChildView(other_bookmarks_button_);

  managed_bookmarks_button_ = CreateManagedBookmarksButton();
  // Also re-enabled when the model is loaded.
  managed_bookmarks_button_->SetEnabled(false);
  AddChildView(managed_bookmarks_button_);

  supervised_bookmarks_button_ = CreateSupervisedBookmarksButton();
  // Also re-enabled when the model is loaded.
  supervised_bookmarks_button_->SetEnabled(false);
  AddChildView(supervised_bookmarks_button_);

  apps_page_shortcut_ = CreateAppsPageShortcutButton();
  AddChildView(apps_page_shortcut_);
  profile_pref_registrar_.Init(browser_->profile()->GetPrefs());
  profile_pref_registrar_.Add(
      bookmarks::prefs::kShowAppsShortcutInBookmarkBar,
      base::Bind(&BookmarkBarView::OnAppsPageShortcutVisibilityPrefChanged,
                 base::Unretained(this)));
  profile_pref_registrar_.Add(
      bookmarks::prefs::kShowManagedBookmarksInBookmarkBar,
      base::Bind(&BookmarkBarView::OnShowManagedBookmarksPrefChanged,
                 base::Unretained(this)));
  apps_page_shortcut_->SetVisible(
      chrome::ShouldShowAppsShortcutInBookmarkBar(
          browser_->profile(), browser_->host_desktop_type()));

  bookmarks_separator_view_ = new ButtonSeparatorView();
  AddChildView(bookmarks_separator_view_);
  UpdateBookmarksSeparatorVisibility();

  instructions_ = new BookmarkBarInstructionsView(this);
  AddChildView(instructions_);

  set_context_menu_controller(this);

  size_animation_.reset(new gfx::SlideAnimation(this));

  model_ = BookmarkModelFactory::GetForProfile(browser_->profile());
  managed_ = ManagedBookmarkServiceFactory::GetForProfile(browser_->profile());
  if (model_) {
    model_->AddObserver(this);
    if (model_->loaded())
      BookmarkModelLoaded(model_, false);
    // else case: we'll receive notification back from the BookmarkModel when
    // done loading, then we'll populate the bar.
  }
}

int BookmarkBarView::GetBookmarkButtonCount() const {
  // We contain seven non-bookmark button views: managed bookmarks, supervised
  // bookmarks, other bookmarks, bookmarks separator, chevrons (for overflow),
  // apps page, and the instruction label.
  return child_count() - 7;
}

views::LabelButton* BookmarkBarView::GetBookmarkButton(int index) {
  // CHECK as otherwise we may do the wrong cast.
  CHECK(index >= 0 && index < GetBookmarkButtonCount());
  return static_cast<views::LabelButton*>(child_at(index));
}

BookmarkLaunchLocation BookmarkBarView::GetBookmarkLaunchLocation() const {
  return IsDetached() ? BOOKMARK_LAUNCH_LOCATION_DETACHED_BAR :
                        BOOKMARK_LAUNCH_LOCATION_ATTACHED_BAR;
}

int BookmarkBarView::GetFirstHiddenNodeIndex() {
  const int bb_count = GetBookmarkButtonCount();
  for (int i = 0; i < bb_count; ++i) {
    if (!GetBookmarkButton(i)->visible())
      return i;
  }
  return bb_count;
}

MenuButton* BookmarkBarView::CreateOtherBookmarksButton() {
  // Title is set in Loaded.
  MenuButton* button =
      new BookmarkFolderButton(this, base::string16(), this, false);
  button->set_id(VIEW_ID_OTHER_BOOKMARKS);
  button->SetImage(views::Button::STATE_NORMAL,
                   *GetImageSkiaNamed(IDR_BOOKMARK_BAR_FOLDER));
  button->set_context_menu_controller(this);
  button->set_tag(kOtherFolderButtonTag);
  return button;
}

MenuButton* BookmarkBarView::CreateManagedBookmarksButton() {
  // Title is set in Loaded.
  MenuButton* button =
      new BookmarkFolderButton(this, base::string16(), this, false);
  button->set_id(VIEW_ID_MANAGED_BOOKMARKS);
  button->SetImage(views::Button::STATE_NORMAL,
                   *GetImageSkiaNamed(IDR_BOOKMARK_BAR_FOLDER_MANAGED));
  button->set_context_menu_controller(this);
  button->set_tag(kManagedFolderButtonTag);
  return button;
}

MenuButton* BookmarkBarView::CreateSupervisedBookmarksButton() {
  // Title is set in Loaded.
  MenuButton* button =
      new BookmarkFolderButton(this, base::string16(), this, false);
  button->set_id(VIEW_ID_SUPERVISED_BOOKMARKS);
  button->SetImage(views::Button::STATE_NORMAL,
                   *GetImageSkiaNamed(IDR_BOOKMARK_BAR_FOLDER_SUPERVISED));
  button->set_context_menu_controller(this);
  button->set_tag(kSupervisedFolderButtonTag);
  return button;
}

MenuButton* BookmarkBarView::CreateOverflowButton() {
  MenuButton* button = new OverFlowButton(this);
  button->SetImage(views::Button::STATE_NORMAL,
                   *GetImageSkiaNamed(IDR_BOOKMARK_BAR_CHEVRONS));

  // The overflow button's image contains an arrow and therefore it is a
  // direction sensitive image and we need to flip it if the UI layout is
  // right-to-left.
  //
  // By default, menu buttons are not flipped because they generally contain
  // text and flipping the gfx::Canvas object will break text rendering. Since
  // the overflow button does not contain text, we can safely flip it.
  button->EnableCanvasFlippingForRTLUI(true);

  // Make visible as necessary.
  button->SetVisible(false);
  // Set accessibility name.
  button->SetAccessibleName(
      l10n_util::GetStringUTF16(IDS_ACCNAME_BOOKMARKS_CHEVRON));
  return button;
}

views::View* BookmarkBarView::CreateBookmarkButton(const BookmarkNode* node) {
  if (node->is_url()) {
    BookmarkButton* button = new BookmarkButton(
        this, node->url(), node->GetTitle(), browser_->profile());
    ConfigureButton(node, button);
    return button;
  }
  views::MenuButton* button =
      new BookmarkFolderButton(this, node->GetTitle(), this, false);
  button->SetImage(views::Button::STATE_NORMAL,
                   *GetImageSkiaNamed(IDR_BOOKMARK_BAR_FOLDER));
  ConfigureButton(node, button);
  return button;
}

views::LabelButton* BookmarkBarView::CreateAppsPageShortcutButton() {
  views::LabelButton* button = new ShortcutButton(
      this, l10n_util::GetStringUTF16(IDS_BOOKMARK_BAR_APPS_SHORTCUT_NAME));
  button->SetTooltipText(l10n_util::GetStringUTF16(
      IDS_BOOKMARK_BAR_APPS_SHORTCUT_TOOLTIP));
  button->set_id(VIEW_ID_BOOKMARK_BAR_ELEMENT);
  button->SetImage(views::Button::STATE_NORMAL,
                   *GetImageSkiaNamed(IDR_BOOKMARK_BAR_APPS_SHORTCUT));
  button->set_context_menu_controller(this);
  button->set_tag(kAppsShortcutButtonTag);
  return button;
}

void BookmarkBarView::ConfigureButton(const BookmarkNode* node,
                                      views::LabelButton* button) {
  button->SetText(node->GetTitle());
  button->SetAccessibleName(node->GetTitle());
  button->set_id(VIEW_ID_BOOKMARK_BAR_ELEMENT);
  // We don't always have a theme provider (ui tests, for example).
  if (GetThemeProvider()) {
    button->SetTextColor(
        views::Button::STATE_NORMAL,
        GetThemeProvider()->GetColor(ThemeProperties::COLOR_BOOKMARK_TEXT));
  }

  button->SetMinSize(gfx::Size());
  button->set_context_menu_controller(this);
  button->set_drag_controller(this);
  if (node->is_url()) {
    const gfx::Image& favicon = model_->GetFavicon(node);
    button->SetImage(views::Button::STATE_NORMAL,
                     favicon.IsEmpty() ? *GetImageSkiaNamed(IDR_DEFAULT_FAVICON)
                                       : *favicon.ToImageSkia());
  }
  button->SetMaxSize(gfx::Size(kMaxButtonWidth, 0));
}

bool BookmarkBarView::BookmarkNodeAddedImpl(BookmarkModel* model,
                                            const BookmarkNode* parent,
                                            int index) {
  const bool needs_layout_and_paint = UpdateOtherAndManagedButtonsVisibility();
  if (parent != model->bookmark_bar_node())
    return needs_layout_and_paint;
  if (index < GetBookmarkButtonCount()) {
    const BookmarkNode* node = parent->GetChild(index);
    AddChildViewAt(CreateBookmarkButton(node), index);
    return true;
  }
  // If the new node was added after the last button we've created we may be
  // able to fit it. Assume we can by returning true, which forces a Layout()
  // and creation of the button (if it fits).
  return index == GetBookmarkButtonCount();
}

bool BookmarkBarView::BookmarkNodeRemovedImpl(BookmarkModel* model,
                                              const BookmarkNode* parent,
                                              int index) {
  const bool needs_layout = UpdateOtherAndManagedButtonsVisibility();

  StopThrobbing(true);
  // No need to start throbbing again as the bookmark bubble can't be up at
  // the same time as the user reorders.

  if (parent != model->bookmark_bar_node()) {
    // Only children of the bookmark_bar_node get buttons.
    return needs_layout;
  }
  if (index >= GetBookmarkButtonCount())
    return needs_layout;

  delete child_at(index);
  return true;
}

void BookmarkBarView::BookmarkNodeChangedImpl(BookmarkModel* model,
                                              const BookmarkNode* node) {
  if (node == managed_->managed_node()) {
    // The managed node may have its title updated.
    managed_bookmarks_button_->SetAccessibleName(
        managed_->managed_node()->GetTitle());
    managed_bookmarks_button_->SetText(managed_->managed_node()->GetTitle());
    return;
  }
  if (node == managed_->supervised_node()) {
    // The supervised node may have its title updated.
    supervised_bookmarks_button_->SetAccessibleName(
        managed_->supervised_node()->GetTitle());
    supervised_bookmarks_button_->SetText(
        managed_->supervised_node()->GetTitle());
    return;
  }

  if (node->parent() != model->bookmark_bar_node()) {
    // We only care about nodes on the bookmark bar.
    return;
  }
  int index = model->bookmark_bar_node()->GetIndexOf(node);
  DCHECK_NE(-1, index);
  if (index >= GetBookmarkButtonCount())
    return;  // Buttons are created as needed.
  views::LabelButton* button = GetBookmarkButton(index);
  const int old_pref_width = button->GetPreferredSize().width();
  ConfigureButton(node, button);
  if (old_pref_width != button->GetPreferredSize().width())
    LayoutAndPaint();
}

void BookmarkBarView::ShowDropFolderForNode(const BookmarkNode* node) {
  if (bookmark_drop_menu_) {
    if (bookmark_drop_menu_->node() == node) {
      // Already showing for the specified node.
      return;
    }
    bookmark_drop_menu_->Cancel();
  }

  views::MenuButton* menu_button = GetMenuButtonForNode(node);
  if (!menu_button)
    return;

  int start_index = 0;
  if (node == model_->bookmark_bar_node())
    start_index = GetFirstHiddenNodeIndex();

  drop_info_->is_menu_showing = true;
  bookmark_drop_menu_ = new BookmarkMenuController(
      browser_, page_navigator_, GetWidget(), node, start_index, true);
  bookmark_drop_menu_->set_observer(this);
  bookmark_drop_menu_->RunMenuAt(this);
}

void BookmarkBarView::StopShowFolderDropMenuTimer() {
  show_folder_method_factory_.InvalidateWeakPtrs();
}

void BookmarkBarView::StartShowFolderDropMenuTimer(const BookmarkNode* node) {
  if (!animations_enabled) {
    // So that tests can run as fast as possible disable the delay during
    // testing.
    ShowDropFolderForNode(node);
    return;
  }
  show_folder_method_factory_.InvalidateWeakPtrs();
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE, base::Bind(&BookmarkBarView::ShowDropFolderForNode,
                            show_folder_method_factory_.GetWeakPtr(), node),
      base::TimeDelta::FromMilliseconds(views::GetMenuShowDelay()));
}

void BookmarkBarView::CalculateDropLocation(const DropTargetEvent& event,
                                            const BookmarkNodeData& data,
                                            DropLocation* location) {
  DCHECK(model_);
  DCHECK(model_->loaded());
  DCHECK(data.is_valid());

  *location = DropLocation();

  // The drop event uses the screen coordinates while the child Views are
  // always laid out from left to right (even though they are rendered from
  // right-to-left on RTL locales). Thus, in order to make sure the drop
  // coordinates calculation works, we mirror the event's X coordinate if the
  // locale is RTL.
  int mirrored_x = GetMirroredXInView(event.x());

  bool found = false;
  const int other_delta_x = mirrored_x - other_bookmarks_button_->x();
  Profile* profile = browser_->profile();
  if (other_bookmarks_button_->visible() && other_delta_x >= 0 &&
      other_delta_x < other_bookmarks_button_->width()) {
    // Mouse is over 'other' folder.
    location->button_type = DROP_OTHER_FOLDER;
    location->on = true;
    found = true;
  } else if (!GetBookmarkButtonCount()) {
    // No bookmarks, accept the drop.
    location->index = 0;
    const BookmarkNode* node = data.GetFirstNode(model_, profile->GetPath());
    int ops = node && managed_->CanBeEditedByUser(node) ?
        ui::DragDropTypes::DRAG_MOVE :
        ui::DragDropTypes::DRAG_COPY | ui::DragDropTypes::DRAG_LINK;
    location->operation = chrome::GetPreferredBookmarkDropOperation(
        event.source_operations(), ops);
    return;
  }

  for (int i = 0; i < GetBookmarkButtonCount() &&
       GetBookmarkButton(i)->visible() && !found; i++) {
    views::LabelButton* button = GetBookmarkButton(i);
    int button_x = mirrored_x - button->x();
    int button_w = button->width();
    if (button_x < button_w) {
      found = true;
      const BookmarkNode* node = model_->bookmark_bar_node()->GetChild(i);
      if (node->is_folder()) {
        if (button_x <= views::kDropBetweenPixels) {
          location->index = i;
        } else if (button_x < button_w - views::kDropBetweenPixels) {
          location->index = i;
          location->on = true;
        } else {
          location->index = i + 1;
        }
      } else if (button_x < button_w / 2) {
        location->index = i;
      } else {
        location->index = i + 1;
      }
      break;
    }
  }

  if (!found) {
    if (overflow_button_->visible()) {
      // Are we over the overflow button?
      int overflow_delta_x = mirrored_x - overflow_button_->x();
      if (overflow_delta_x >= 0 &&
          overflow_delta_x < overflow_button_->width()) {
        // Mouse is over overflow button.
        location->index = GetFirstHiddenNodeIndex();
        location->button_type = DROP_OVERFLOW;
      } else if (overflow_delta_x < 0) {
        // Mouse is after the last visible button but before overflow button;
        // use the last visible index.
        location->index = GetFirstHiddenNodeIndex();
      } else {
        return;
      }
    } else if (!other_bookmarks_button_->visible() ||
               mirrored_x < other_bookmarks_button_->x()) {
      // Mouse is after the last visible button but before more recently
      // bookmarked; use the last visible index.
      location->index = GetFirstHiddenNodeIndex();
    } else {
      return;
    }
  }

  if (location->on) {
    const BookmarkNode* parent = (location->button_type == DROP_OTHER_FOLDER) ?
        model_->other_node() :
        model_->bookmark_bar_node()->GetChild(location->index);
    location->operation = chrome::GetBookmarkDropOperation(
        profile, event, data, parent, parent->child_count());
    if (!location->operation && !data.has_single_url() &&
        data.GetFirstNode(model_, profile->GetPath()) == parent) {
      // Don't open a menu if the node being dragged is the menu to open.
      location->on = false;
    }
  } else {
    location->operation = chrome::GetBookmarkDropOperation(
        profile, event, data, model_->bookmark_bar_node(), location->index);
  }
}

void BookmarkBarView::WriteBookmarkDragData(const BookmarkNode* node,
                                            ui::OSExchangeData* data) {
  DCHECK(node && data);
  BookmarkNodeData drag_data(node);
  drag_data.Write(browser_->profile()->GetPath(), data);
}

void BookmarkBarView::StartThrobbing(const BookmarkNode* node,
                                     bool overflow_only) {
  DCHECK(!throbbing_view_);

  // Determine which visible button is showing the bookmark (or is an ancestor
  // of the bookmark).
  const BookmarkNode* bbn = model_->bookmark_bar_node();
  const BookmarkNode* parent_on_bb = node;
  while (parent_on_bb) {
    const BookmarkNode* parent = parent_on_bb->parent();
    if (parent == bbn)
      break;
    parent_on_bb = parent;
  }
  if (parent_on_bb) {
    int index = bbn->GetIndexOf(parent_on_bb);
    if (index >= GetFirstHiddenNodeIndex()) {
      // Node is hidden, animate the overflow button.
      throbbing_view_ = overflow_button_;
    } else if (!overflow_only) {
      throbbing_view_ = static_cast<CustomButton*>(child_at(index));
    }
  } else if (bookmarks::IsDescendantOf(node, managed_->managed_node())) {
    throbbing_view_ = managed_bookmarks_button_;
  } else if (bookmarks::IsDescendantOf(node, managed_->supervised_node())) {
    throbbing_view_ = supervised_bookmarks_button_;
  } else if (!overflow_only) {
    throbbing_view_ = other_bookmarks_button_;
  }

  // Use a large number so that the button continues to throb.
  if (throbbing_view_)
    throbbing_view_->StartThrobbing(std::numeric_limits<int>::max());
}

views::CustomButton* BookmarkBarView::DetermineViewToThrobFromRemove(
    const BookmarkNode* parent,
    int old_index) {
  const BookmarkNode* bbn = model_->bookmark_bar_node();
  const BookmarkNode* old_node = parent;
  int old_index_on_bb = old_index;
  while (old_node && old_node != bbn) {
    const BookmarkNode* parent = old_node->parent();
    if (parent == bbn) {
      old_index_on_bb = bbn->GetIndexOf(old_node);
      break;
    }
    old_node = parent;
  }
  if (old_node) {
    if (old_index_on_bb >= GetFirstHiddenNodeIndex()) {
      // Node is hidden, animate the overflow button.
      return overflow_button_;
    }
    return static_cast<CustomButton*>(child_at(old_index_on_bb));
  }
  if (bookmarks::IsDescendantOf(parent, managed_->managed_node()))
    return managed_bookmarks_button_;
  if (bookmarks::IsDescendantOf(parent, managed_->supervised_node()))
    return supervised_bookmarks_button_;
  // Node wasn't on the bookmark bar, use the "Other Bookmarks" button.
  return other_bookmarks_button_;
}

void BookmarkBarView::UpdateColors() {
  // We don't always have a theme provider (ui tests, for example).
  const ui::ThemeProvider* theme_provider = GetThemeProvider();
  if (!theme_provider)
    return;
  SkColor color =
      theme_provider->GetColor(ThemeProperties::COLOR_BOOKMARK_TEXT);
  for (int i = 0; i < GetBookmarkButtonCount(); ++i)
    GetBookmarkButton(i)->SetTextColor(views::Button::STATE_NORMAL, color);
  other_bookmarks_button_->SetTextColor(views::Button::STATE_NORMAL, color);
  managed_bookmarks_button_->SetTextColor(views::Button::STATE_NORMAL, color);
  supervised_bookmarks_button_->SetTextColor(views::Button::STATE_NORMAL,
                                             color);
  if (apps_page_shortcut_->visible())
    apps_page_shortcut_->SetTextColor(views::Button::STATE_NORMAL, color);
}

bool BookmarkBarView::UpdateOtherAndManagedButtonsVisibility() {
  bool has_other_children = !model_->other_node()->empty();
  bool update_other = has_other_children != other_bookmarks_button_->visible();
  if (update_other) {
    other_bookmarks_button_->SetVisible(has_other_children);
    UpdateBookmarksSeparatorVisibility();
  }

  bool show_managed = !managed_->managed_node()->empty() &&
                      browser_->profile()->GetPrefs()->GetBoolean(
                          bookmarks::prefs::kShowManagedBookmarksInBookmarkBar);
  bool update_managed = show_managed != managed_bookmarks_button_->visible();
  if (update_managed)
    managed_bookmarks_button_->SetVisible(show_managed);

  bool show_supervised = !managed_->supervised_node()->empty();
  bool update_supervised =
      show_supervised != supervised_bookmarks_button_->visible();
  if (update_supervised)
    supervised_bookmarks_button_->SetVisible(show_supervised);

  return update_other || update_managed || update_supervised;
}

void BookmarkBarView::UpdateBookmarksSeparatorVisibility() {
  // Ash does not paint the bookmarks separator line because it looks odd on
  // the flat background.  We keep it present for layout, but don't draw it.
  bookmarks_separator_view_->SetVisible(
      browser_->host_desktop_type() != chrome::HOST_DESKTOP_TYPE_ASH &&
      other_bookmarks_button_->visible());
}

void BookmarkBarView::OnAppsPageShortcutVisibilityPrefChanged() {
  DCHECK(apps_page_shortcut_);
  // Only perform layout if required.
  bool visible = chrome::ShouldShowAppsShortcutInBookmarkBar(
      browser_->profile(), browser_->host_desktop_type());
  if (apps_page_shortcut_->visible() == visible)
    return;
  apps_page_shortcut_->SetVisible(visible);
  UpdateBookmarksSeparatorVisibility();
  LayoutAndPaint();
}

void BookmarkBarView::OnShowManagedBookmarksPrefChanged() {
  if (UpdateOtherAndManagedButtonsVisibility())
    LayoutAndPaint();
}
