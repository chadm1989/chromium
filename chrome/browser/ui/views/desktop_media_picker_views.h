// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_DESKTOP_MEDIA_PICKER_VIEWS_H_
#define CHROME_BROWSER_UI_VIEWS_DESKTOP_MEDIA_PICKER_VIEWS_H_

#include "chrome/browser/media/desktop_media_list_observer.h"
#include "chrome/browser/media/desktop_media_picker.h"
#include "ui/views/window/dialog_delegate.h"

namespace views {
class ImageView;
class Label;
}  // namespace views

class DesktopMediaPickerDialogView;
class DesktopMediaPickerViews;
class DesktopMediaSourceView;

// View that shows a list of desktop media sources available from
// DesktopMediaList.
class DesktopMediaListView : public views::View,
                             public DesktopMediaListObserver {
 public:
  DesktopMediaListView(DesktopMediaPickerDialogView* parent,
                       scoped_ptr<DesktopMediaList> media_list);
  ~DesktopMediaListView() override;

  void StartUpdating(content::DesktopMediaID dialog_window_id);

  // Called by DesktopMediaSourceView when selection has changed.
  void OnSelectionChanged();

  // Called by DesktopMediaSourceView when a source has been double-clicked.
  void OnDoubleClick();

  // Returns currently selected source.
  DesktopMediaSourceView* GetSelection();

  // views::View overrides.
  gfx::Size GetPreferredSize() const override;
  void Layout() override;
  bool OnKeyPressed(const ui::KeyEvent& event) override;

 private:
  // DesktopMediaList::Observer interface
  void OnSourceAdded(int index) override;
  void OnSourceRemoved(int index) override;
  void OnSourceMoved(int old_index, int new_index) override;
  void OnSourceNameChanged(int index) override;
  void OnSourceThumbnailChanged(int index) override;

  // Accepts whatever happens to be selected right now.
  void AcceptSelection();

  DesktopMediaPickerDialogView* parent_;
  scoped_ptr<DesktopMediaList> media_list_;
  base::WeakPtrFactory<DesktopMediaListView> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(DesktopMediaListView);
};

// View used for each item in DesktopMediaListView. Shows a single desktop media
// source as a thumbnail with the title under it.
class DesktopMediaSourceView : public views::View {
 public:
  DesktopMediaSourceView(DesktopMediaListView* parent,
                         content::DesktopMediaID source_id);
  ~DesktopMediaSourceView() override;

  // Updates thumbnail and title from |source|.
  void SetName(const base::string16& name);
  void SetThumbnail(const gfx::ImageSkia& thumbnail);

  // Id for the source shown by this View.
  const content::DesktopMediaID& source_id() const { return source_id_; }

  // Returns true if the source is selected.
  bool is_selected() const { return selected_; }

  // views::View interface.
  const char* GetClassName() const override;
  void Layout() override;
  views::View* GetSelectedViewForGroup(int group) override;
  bool IsGroupFocusTraversable() const override;
  void OnPaint(gfx::Canvas* canvas) override;
  void OnFocus() override;
  void OnBlur() override;
  bool OnMousePressed(const ui::MouseEvent& event) override;
  void OnGestureEvent(ui::GestureEvent* event) override;

 private:
  // Updates selection state of the element. If |selected| is true then also
  // calls SetSelected(false) for the source view that was selected before that
  // (if any).
  void SetSelected(bool selected);

  DesktopMediaListView* parent_;
  content::DesktopMediaID source_id_;

  views::ImageView* image_view_;
  views::Label* label_;

  bool selected_;

  DISALLOW_COPY_AND_ASSIGN(DesktopMediaSourceView);
};

// Dialog view used for DesktopMediaPickerViews.
class DesktopMediaPickerDialogView : public views::DialogDelegateView {
 public:
  DesktopMediaPickerDialogView(content::WebContents* parent_web_contents,
                               gfx::NativeWindow context,
                               DesktopMediaPickerViews* parent,
                               const base::string16& app_name,
                               const base::string16& target_name,
                               scoped_ptr<DesktopMediaList> media_list);
  ~DesktopMediaPickerDialogView() override;

  // Called by parent (DesktopMediaPickerViews) when it's destroyed.
  void DetachParent();

  // Called by DesktopMediaListView.
  void OnSelectionChanged();
  void OnDoubleClick();

  // views::View overrides.
  gfx::Size GetPreferredSize() const override;
  void Layout() override;

  // views::DialogDelegateView overrides.
  ui::ModalType GetModalType() const override;
  base::string16 GetWindowTitle() const override;
  bool IsDialogButtonEnabled(ui::DialogButton button) const override;
  base::string16 GetDialogButtonLabel(ui::DialogButton button) const override;
  bool Accept() override;
  void DeleteDelegate() override;

  void OnMediaListRowsChanged();

  DesktopMediaSourceView* GetMediaSourceViewForTesting(int index) const;

 private:
  DesktopMediaPickerViews* parent_;
  base::string16 app_name_;

  views::Label* label_;
  views::ScrollView* scroll_view_;
  DesktopMediaListView* list_view_;

  DISALLOW_COPY_AND_ASSIGN(DesktopMediaPickerDialogView);
};

// Implementation of DesktopMediaPicker for Views.
class DesktopMediaPickerViews : public DesktopMediaPicker {
 public:
  DesktopMediaPickerViews();
  ~DesktopMediaPickerViews() override;

  void NotifyDialogResult(content::DesktopMediaID source);

  // DesktopMediaPicker overrides.
  void Show(content::WebContents* web_contents,
            gfx::NativeWindow context,
            gfx::NativeWindow parent,
            const base::string16& app_name,
            const base::string16& target_name,
            scoped_ptr<DesktopMediaList> media_list,
            const DoneCallback& done_callback) override;

  DesktopMediaPickerDialogView* GetDialogViewForTesting() const {
    return dialog_;
  }

 private:
  DoneCallback callback_;

  // The |dialog_| is owned by the corresponding views::Widget instance.
  // When DesktopMediaPickerViews is destroyed the |dialog_| is destroyed
  // asynchronously by closing the widget.
  DesktopMediaPickerDialogView* dialog_;

  DISALLOW_COPY_AND_ASSIGN(DesktopMediaPickerViews);
};

#endif  // CHROME_BROWSER_UI_VIEWS_DESKTOP_MEDIA_PICKER_VIEWS_H_
