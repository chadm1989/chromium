// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/scoped_ptr.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/bubble/bubble_border.h"
#include "ui/views/bubble/bubble_frame_view.h"
#include "ui/views/test/test_views.h"
#include "ui/views/test/views_test_base.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"

namespace views {

typedef ViewsTestBase BubbleFrameViewTest;

namespace {

const BubbleBorder::Arrow kArrow = BubbleBorder::TOP_LEFT;
const SkColor kColor = SK_ColorRED;
const int kMargin = 6;
const int kMinimumClientWidth = 100;
const int kMinimumClientHeight = 200;
const int kMaximumClientWidth = 300;
const int kMaximumClientHeight = 300;
const int kPreferredClientWidth = 150;
const int kPreferredClientHeight = 250;
const int kExpectedBorderWidth = 22;
const int kExpectedBorderHeight = 29;

class TestBubbleFrameViewWidgetDelegate : public WidgetDelegate {
 public:
  TestBubbleFrameViewWidgetDelegate(Widget* widget) : widget_(widget) {}

  ~TestBubbleFrameViewWidgetDelegate() override {}

  // WidgetDelegate overrides:
  Widget* GetWidget() override { return widget_; }
  const Widget* GetWidget() const override { return widget_; }

  View* GetContentsView() override {
    if (!contents_view_) {
      StaticSizedView* contents_view = new StaticSizedView(
          gfx::Size(kPreferredClientWidth, kPreferredClientHeight));
      contents_view->set_minimum_size(
          gfx::Size(kMinimumClientWidth, kMinimumClientHeight));
      contents_view->set_maximum_size(
          gfx::Size(kMaximumClientWidth, kMaximumClientHeight));
      contents_view_ = contents_view;
    }
    return contents_view_;
  }

 private:
  Widget* widget_;
  View* contents_view_ = nullptr;  // Owned by |widget_|.
};

class TestBubbleFrameView : public BubbleFrameView {
 public:
  TestBubbleFrameView(ViewsTestBase* test_base)
      : BubbleFrameView(gfx::Insets(kMargin, kMargin, kMargin, kMargin)),
        test_base_(test_base),
        available_bounds_(gfx::Rect(0, 0, 1000, 1000)) {
    SetBubbleBorder(scoped_ptr<BubbleBorder>(
        new BubbleBorder(kArrow, BubbleBorder::NO_SHADOW, kColor)));
  }
  ~TestBubbleFrameView() override {}

  // View overrides:
  const Widget* GetWidget() const override {
    if (!widget_) {
      widget_.reset(new Widget);
      widget_delegate_.reset(
          new TestBubbleFrameViewWidgetDelegate(widget_.get()));
      Widget::InitParams params =
          test_base_->CreateParams(Widget::InitParams::TYPE_BUBBLE);
      params.delegate = widget_delegate_.get();
      params.ownership = Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
      widget_->Init(params);
    }

    return widget_.get();
  }

  // BubbleFrameView overrides:
  gfx::Rect GetAvailableScreenBounds(const gfx::Rect& rect) override {
    return available_bounds_;
  }

 private:
  ViewsTestBase* test_base_;

  gfx::Rect available_bounds_;

  // Widget returned by GetWidget(). Only created if GetWidget() is called.
  mutable scoped_ptr<TestBubbleFrameViewWidgetDelegate> widget_delegate_;
  mutable scoped_ptr<Widget> widget_;

  DISALLOW_COPY_AND_ASSIGN(TestBubbleFrameView);
};

}  // namespace

TEST_F(BubbleFrameViewTest, GetBoundsForClientView) {
  TestBubbleFrameView frame(this);
  EXPECT_EQ(kArrow, frame.bubble_border()->arrow());
  EXPECT_EQ(kColor, frame.bubble_border()->background_color());

  int margin_x = frame.content_margins().left();
  int margin_y = frame.content_margins().top();
  gfx::Insets insets = frame.bubble_border()->GetInsets();
  EXPECT_EQ(insets.left() + margin_x, frame.GetBoundsForClientView().x());
  EXPECT_EQ(insets.top() + margin_y, frame.GetBoundsForClientView().y());
}

// Tests that the arrow is mirrored as needed to better fit the screen.
TEST_F(BubbleFrameViewTest, GetUpdatedWindowBounds) {
  TestBubbleFrameView frame(this);
  gfx::Rect window_bounds;

  gfx::Insets insets = frame.bubble_border()->GetInsets();
  int xposition = 95 - insets.width();

  // Test that the info bubble displays normally when it fits.
  frame.bubble_border()->set_arrow(BubbleBorder::TOP_LEFT);
  window_bounds = frame.GetUpdatedWindowBounds(
      gfx::Rect(100, 100, 50, 50),  // |anchor_rect|
      gfx::Size(500, 500),          // |client_size|
      true);                        // |adjust_if_offscreen|
  EXPECT_EQ(BubbleBorder::TOP_LEFT, frame.bubble_border()->arrow());
  EXPECT_GT(window_bounds.x(), xposition);
  EXPECT_GT(window_bounds.y(), 100 + 50 - 10);  // -10 to roughly compensate for
                                                // arrow overlap.

  // Test bubble not fitting on left.
  frame.bubble_border()->set_arrow(BubbleBorder::TOP_RIGHT);
  window_bounds = frame.GetUpdatedWindowBounds(
      gfx::Rect(100, 100, 50, 50),  // |anchor_rect|
      gfx::Size(500, 500),          // |client_size|
      true);                        // |adjust_if_offscreen|
  EXPECT_EQ(BubbleBorder::TOP_LEFT, frame.bubble_border()->arrow());
  EXPECT_GT(window_bounds.x(), xposition);
  EXPECT_GT(window_bounds.y(), 100 + 50 - 10);  // -10 to roughly compensate for
                                                // arrow overlap.

  // Test bubble not fitting on left or top.
  frame.bubble_border()->set_arrow(BubbleBorder::BOTTOM_RIGHT);
  window_bounds = frame.GetUpdatedWindowBounds(
      gfx::Rect(100, 100, 50, 50),  // |anchor_rect|
      gfx::Size(500, 500),          // |client_size|
      true);                        // |adjust_if_offscreen|
  EXPECT_EQ(BubbleBorder::TOP_LEFT, frame.bubble_border()->arrow());
  EXPECT_GT(window_bounds.x(), xposition);
  EXPECT_GT(window_bounds.y(), 100 + 50 - 10);  // -10 to roughly compensate for
                                                // arrow overlap.

  // Test bubble not fitting on top.
  frame.bubble_border()->set_arrow(BubbleBorder::BOTTOM_LEFT);
  window_bounds = frame.GetUpdatedWindowBounds(
      gfx::Rect(100, 100, 50, 50),  // |anchor_rect|
      gfx::Size(500, 500),          // |client_size|
      true);                        // |adjust_if_offscreen|
  EXPECT_EQ(BubbleBorder::TOP_LEFT, frame.bubble_border()->arrow());
  EXPECT_GT(window_bounds.x(), xposition);
  EXPECT_GT(window_bounds.y(), 100 + 50 - 10);  // -10 to roughly compensate for
                                                // arrow overlap.

  // Test bubble not fitting on top and right.
  frame.bubble_border()->set_arrow(BubbleBorder::BOTTOM_LEFT);
  window_bounds = frame.GetUpdatedWindowBounds(
      gfx::Rect(900, 100, 50, 50),  // |anchor_rect|
      gfx::Size(500, 500),          // |client_size|
      true);                        // |adjust_if_offscreen|
  EXPECT_EQ(BubbleBorder::TOP_RIGHT, frame.bubble_border()->arrow());
  EXPECT_LT(window_bounds.x(), 900 + 50 - 500);
  EXPECT_GT(window_bounds.y(), 100 + 50 - 10);  // -10 to roughly compensate for
                                                // arrow overlap.

  // Test bubble not fitting on right.
  frame.bubble_border()->set_arrow(BubbleBorder::TOP_LEFT);
  window_bounds = frame.GetUpdatedWindowBounds(
      gfx::Rect(900, 100, 50, 50),  // |anchor_rect|
      gfx::Size(500, 500),          // |client_size|
      true);                        // |adjust_if_offscreen|
  EXPECT_EQ(BubbleBorder::TOP_RIGHT, frame.bubble_border()->arrow());
  EXPECT_LT(window_bounds.x(), 900 + 50 - 500);
  EXPECT_GT(window_bounds.y(), 100 + 50 - 10);  // -10 to roughly compensate for
                                                // arrow overlap.

  // Test bubble not fitting on bottom and right.
  frame.bubble_border()->set_arrow(BubbleBorder::TOP_LEFT);
  window_bounds = frame.GetUpdatedWindowBounds(
      gfx::Rect(900, 900, 50, 50),  // |anchor_rect|
      gfx::Size(500, 500),          // |client_size|
      true);                        // |adjust_if_offscreen|
  EXPECT_EQ(BubbleBorder::BOTTOM_RIGHT, frame.bubble_border()->arrow());
  EXPECT_LT(window_bounds.x(), 900 + 50 - 500);
  EXPECT_LT(window_bounds.y(), 900 - 500 - 15);  // -15 to roughly compensate
                                                 // for arrow height.

  // Test bubble not fitting at the bottom.
  frame.bubble_border()->set_arrow(BubbleBorder::TOP_LEFT);
  window_bounds = frame.GetUpdatedWindowBounds(
      gfx::Rect(100, 900, 50, 50),  // |anchor_rect|
      gfx::Size(500, 500),          // |client_size|
      true);                        // |adjust_if_offscreen|
  EXPECT_EQ(BubbleBorder::BOTTOM_LEFT, frame.bubble_border()->arrow());
  // The window should be right aligned with the anchor_rect.
  EXPECT_LT(window_bounds.x(), 900 + 50 - 500);
  EXPECT_LT(window_bounds.y(), 900 - 500 - 15);  // -15 to roughly compensate
                                                 // for arrow height.

  // Test bubble not fitting at the bottom and left.
  frame.bubble_border()->set_arrow(BubbleBorder::TOP_RIGHT);
  window_bounds = frame.GetUpdatedWindowBounds(
      gfx::Rect(100, 900, 50, 50),  // |anchor_rect|
      gfx::Size(500, 500),          // |client_size|
      true);                        // |adjust_if_offscreen|
  EXPECT_EQ(BubbleBorder::BOTTOM_LEFT, frame.bubble_border()->arrow());
  // The window should be right aligned with the anchor_rect.
  EXPECT_LT(window_bounds.x(), 900 + 50 - 500);
  EXPECT_LT(window_bounds.y(), 900 - 500 - 15);  // -15 to roughly compensate
                                                 // for arrow height.
}

// Tests that the arrow is not moved when the info-bubble does not fit the
// screen but moving it would make matter worse.
TEST_F(BubbleFrameViewTest, GetUpdatedWindowBoundsMirroringFails) {
  TestBubbleFrameView frame(this);
  frame.bubble_border()->set_arrow(BubbleBorder::TOP_LEFT);
  gfx::Rect window_bounds = frame.GetUpdatedWindowBounds(
      gfx::Rect(400, 100, 50, 50),  // |anchor_rect|
      gfx::Size(500, 700),          // |client_size|
      true);                        // |adjust_if_offscreen|
  EXPECT_EQ(BubbleBorder::TOP_LEFT, frame.bubble_border()->arrow());
}

TEST_F(BubbleFrameViewTest, TestMirroringForCenteredArrow) {
  TestBubbleFrameView frame(this);

  // Test bubble not fitting above the anchor.
  frame.bubble_border()->set_arrow(BubbleBorder::BOTTOM_CENTER);
  gfx::Rect window_bounds = frame.GetUpdatedWindowBounds(
      gfx::Rect(100, 100, 50, 50),  // |anchor_rect|
      gfx::Size(500, 700),          // |client_size|
      true);                        // |adjust_if_offscreen|
  EXPECT_EQ(BubbleBorder::TOP_CENTER, frame.bubble_border()->arrow());

  // Test bubble not fitting below the anchor.
  frame.bubble_border()->set_arrow(BubbleBorder::TOP_CENTER);
  window_bounds = frame.GetUpdatedWindowBounds(
      gfx::Rect(300, 800, 50, 50),  // |anchor_rect|
      gfx::Size(500, 200),          // |client_size|
      true);                        // |adjust_if_offscreen|
  EXPECT_EQ(BubbleBorder::BOTTOM_CENTER, frame.bubble_border()->arrow());

  // Test bubble not fitting to the right of the anchor.
  frame.bubble_border()->set_arrow(BubbleBorder::LEFT_CENTER);
  window_bounds = frame.GetUpdatedWindowBounds(
      gfx::Rect(800, 300, 50, 50),  // |anchor_rect|
      gfx::Size(200, 500),          // |client_size|
      true);                        // |adjust_if_offscreen|
  EXPECT_EQ(BubbleBorder::RIGHT_CENTER, frame.bubble_border()->arrow());

  // Test bubble not fitting to the left of the anchor.
  frame.bubble_border()->set_arrow(BubbleBorder::RIGHT_CENTER);
  window_bounds = frame.GetUpdatedWindowBounds(
      gfx::Rect(100, 300, 50, 50),  // |anchor_rect|
      gfx::Size(500, 500),          // |client_size|
      true);                        // |adjust_if_offscreen|
  EXPECT_EQ(BubbleBorder::LEFT_CENTER, frame.bubble_border()->arrow());
}

// Test that the arrow will not be mirrored when |adjust_if_offscreen| is false.
TEST_F(BubbleFrameViewTest, GetUpdatedWindowBoundsDontTryMirror) {
  TestBubbleFrameView frame(this);
  frame.bubble_border()->set_arrow(BubbleBorder::TOP_RIGHT);
  gfx::Rect window_bounds = frame.GetUpdatedWindowBounds(
      gfx::Rect(100, 900, 50, 50),  // |anchor_rect|
      gfx::Size(500, 500),          // |client_size|
      false);                       // |adjust_if_offscreen|
  EXPECT_EQ(BubbleBorder::TOP_RIGHT, frame.bubble_border()->arrow());
  // The coordinates should be pointing to anchor_rect from TOP_RIGHT.
  EXPECT_LT(window_bounds.x(), 100 + 50 - 500);
  EXPECT_GT(window_bounds.y(), 900 + 50 - 10);  // -10 to roughly compensate for
                                                // arrow overlap.
}

// Test that the center arrow is moved as needed to fit the screen.
TEST_F(BubbleFrameViewTest, GetUpdatedWindowBoundsCenterArrows) {
  TestBubbleFrameView frame(this);
  gfx::Rect window_bounds;

  // Test that the bubble displays normally when it fits.
  frame.bubble_border()->set_arrow(BubbleBorder::TOP_CENTER);
  window_bounds = frame.GetUpdatedWindowBounds(
      gfx::Rect(500, 100, 50, 50),  // |anchor_rect|
      gfx::Size(500, 500),          // |client_size|
      true);                        // |adjust_if_offscreen|
  EXPECT_EQ(BubbleBorder::TOP_CENTER, frame.bubble_border()->arrow());
  EXPECT_EQ(window_bounds.x() + window_bounds.width() / 2, 525);

  frame.bubble_border()->set_arrow(BubbleBorder::BOTTOM_CENTER);
  window_bounds = frame.GetUpdatedWindowBounds(
      gfx::Rect(500, 900, 50, 50),  // |anchor_rect|
      gfx::Size(500, 500),          // |client_size|
      true);                        // |adjust_if_offscreen|
  EXPECT_EQ(BubbleBorder::BOTTOM_CENTER, frame.bubble_border()->arrow());
  EXPECT_EQ(window_bounds.x() + window_bounds.width() / 2, 525);

  frame.bubble_border()->set_arrow(BubbleBorder::LEFT_CENTER);
  window_bounds = frame.GetUpdatedWindowBounds(
      gfx::Rect(100, 400, 50, 50),  // |anchor_rect|
      gfx::Size(500, 500),          // |client_size|
      true);                        // |adjust_if_offscreen|
  EXPECT_EQ(BubbleBorder::LEFT_CENTER, frame.bubble_border()->arrow());
  EXPECT_EQ(window_bounds.y() + window_bounds.height() / 2, 425);

  frame.bubble_border()->set_arrow(BubbleBorder::RIGHT_CENTER);
  window_bounds = frame.GetUpdatedWindowBounds(
      gfx::Rect(900, 400, 50, 50),  // |anchor_rect|
      gfx::Size(500, 500),          // |client_size|
      true);                        // |adjust_if_offscreen|
  EXPECT_EQ(BubbleBorder::RIGHT_CENTER, frame.bubble_border()->arrow());
  EXPECT_EQ(window_bounds.y() + window_bounds.height() / 2, 425);

  // Test bubble not fitting left screen edge.
  frame.bubble_border()->set_arrow(BubbleBorder::TOP_CENTER);
  window_bounds = frame.GetUpdatedWindowBounds(
      gfx::Rect(100, 100, 50, 50),  // |anchor_rect|
      gfx::Size(500, 500),          // |client_size|
      true);                        // |adjust_if_offscreen|
  EXPECT_EQ(BubbleBorder::TOP_CENTER, frame.bubble_border()->arrow());
  EXPECT_EQ(window_bounds.x(), 0);
  EXPECT_EQ(window_bounds.x() +
            frame.bubble_border()->GetArrowOffset(window_bounds.size()), 125);

  frame.bubble_border()->set_arrow(BubbleBorder::BOTTOM_CENTER);
  window_bounds = frame.GetUpdatedWindowBounds(
      gfx::Rect(100, 900, 50, 50),  // |anchor_rect|
      gfx::Size(500, 500),          // |client_size|
      true);                        // |adjust_if_offscreen|
  EXPECT_EQ(BubbleBorder::BOTTOM_CENTER, frame.bubble_border()->arrow());
  EXPECT_EQ(window_bounds.x(), 0);
  EXPECT_EQ(window_bounds.x() +
            frame.bubble_border()->GetArrowOffset(window_bounds.size()), 125);

  // Test bubble not fitting right screen edge.
  frame.bubble_border()->set_arrow(BubbleBorder::TOP_CENTER);
  window_bounds = frame.GetUpdatedWindowBounds(
      gfx::Rect(900, 100, 50, 50),  // |anchor_rect|
      gfx::Size(500, 500),          // |client_size|
      true);                        // |adjust_if_offscreen|
  EXPECT_EQ(BubbleBorder::TOP_CENTER, frame.bubble_border()->arrow());
  EXPECT_EQ(window_bounds.right(), 1000);
  EXPECT_EQ(window_bounds.x() +
            frame.bubble_border()->GetArrowOffset(window_bounds.size()), 925);

  frame.bubble_border()->set_arrow(BubbleBorder::BOTTOM_CENTER);
  window_bounds = frame.GetUpdatedWindowBounds(
      gfx::Rect(900, 900, 50, 50),  // |anchor_rect|
      gfx::Size(500, 500),          // |client_size|
      true);                        // |adjust_if_offscreen|
  EXPECT_EQ(BubbleBorder::BOTTOM_CENTER, frame.bubble_border()->arrow());
  EXPECT_EQ(window_bounds.right(), 1000);
  EXPECT_EQ(window_bounds.x() +
            frame.bubble_border()->GetArrowOffset(window_bounds.size()), 925);

  // Test bubble not fitting top screen edge.
  frame.bubble_border()->set_arrow(BubbleBorder::LEFT_CENTER);
  window_bounds = frame.GetUpdatedWindowBounds(
      gfx::Rect(100, 100, 50, 50),  // |anchor_rect|
      gfx::Size(500, 500),          // |client_size|
      true);                        // |adjust_if_offscreen|
  EXPECT_EQ(BubbleBorder::LEFT_CENTER, frame.bubble_border()->arrow());
  EXPECT_EQ(window_bounds.y(), 0);
  EXPECT_EQ(window_bounds.y() +
            frame.bubble_border()->GetArrowOffset(window_bounds.size()), 125);

  frame.bubble_border()->set_arrow(BubbleBorder::RIGHT_CENTER);
  window_bounds = frame.GetUpdatedWindowBounds(
      gfx::Rect(900, 100, 50, 50),  // |anchor_rect|
      gfx::Size(500, 500),          // |client_size|
      true);                        // |adjust_if_offscreen|
  EXPECT_EQ(BubbleBorder::RIGHT_CENTER, frame.bubble_border()->arrow());
  EXPECT_EQ(window_bounds.y(), 0);
  EXPECT_EQ(window_bounds.y() +
            frame.bubble_border()->GetArrowOffset(window_bounds.size()), 125);

  // Test bubble not fitting bottom screen edge.
  frame.bubble_border()->set_arrow(BubbleBorder::LEFT_CENTER);
  window_bounds = frame.GetUpdatedWindowBounds(
      gfx::Rect(100, 900, 50, 50),  // |anchor_rect|
      gfx::Size(500, 500),          // |client_size|
      true);                        // |adjust_if_offscreen|
  EXPECT_EQ(BubbleBorder::LEFT_CENTER, frame.bubble_border()->arrow());
  EXPECT_EQ(window_bounds.bottom(), 1000);
  EXPECT_EQ(window_bounds.y() +
            frame.bubble_border()->GetArrowOffset(window_bounds.size()), 925);

  frame.bubble_border()->set_arrow(BubbleBorder::RIGHT_CENTER);
  window_bounds = frame.GetUpdatedWindowBounds(
      gfx::Rect(900, 900, 50, 50),  // |anchor_rect|
      gfx::Size(500, 500),          // |client_size|
      true);                        // |adjust_if_offscreen|
  EXPECT_EQ(BubbleBorder::RIGHT_CENTER, frame.bubble_border()->arrow());
  EXPECT_EQ(window_bounds.bottom(), 1000);
  EXPECT_EQ(window_bounds.y() +
            frame.bubble_border()->GetArrowOffset(window_bounds.size()), 925);
}

TEST_F(BubbleFrameViewTest, GetPreferredSize) {
  TestBubbleFrameView frame(this);
  gfx::Size preferred_size = frame.GetPreferredSize();
  // Expect that a border has been added to the preferred size.
  EXPECT_EQ(kPreferredClientWidth + kExpectedBorderWidth,
            preferred_size.width());
  EXPECT_EQ(kPreferredClientHeight + kExpectedBorderHeight,
            preferred_size.height());
}

TEST_F(BubbleFrameViewTest, GetMinimumSize) {
  TestBubbleFrameView frame(this);
  gfx::Size minimum_size = frame.GetMinimumSize();
  // Expect that a border has been added to the minimum size.
  EXPECT_EQ(kMinimumClientWidth + kExpectedBorderWidth, minimum_size.width());
  EXPECT_EQ(kMinimumClientHeight + kExpectedBorderHeight,
            minimum_size.height());
}

TEST_F(BubbleFrameViewTest, GetMaximumSize) {
  TestBubbleFrameView frame(this);
  gfx::Size maximum_size = frame.GetMaximumSize();
  // Should ignore the contents view's maximum size and use the preferred size.
  EXPECT_EQ(kPreferredClientWidth + kExpectedBorderWidth, maximum_size.width());
  EXPECT_EQ(kPreferredClientHeight + kExpectedBorderHeight,
            maximum_size.height());
}

}  // namespace views
