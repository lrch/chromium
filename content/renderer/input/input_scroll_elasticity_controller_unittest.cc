// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/input/input_handler.h"
#include "content/renderer/input/input_scroll_elasticity_controller.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/WebKit/public/web/WebInputEvent.h"

namespace content {
namespace {

enum Phase {
  PhaseNone = blink::WebMouseWheelEvent::PhaseNone,
  PhaseBegan = blink::WebMouseWheelEvent::PhaseBegan,
  PhaseStationary = blink::WebMouseWheelEvent::PhaseStationary,
  PhaseChanged = blink::WebMouseWheelEvent::PhaseChanged,
  PhaseEnded = blink::WebMouseWheelEvent::PhaseEnded,
  PhaseCancelled = blink::WebMouseWheelEvent::PhaseCancelled,
  PhaseMayBegin = blink::WebMouseWheelEvent::PhaseMayBegin,
};

class MockScrollElasticityHelper : public cc::ScrollElasticityHelper {
 public:
  MockScrollElasticityHelper()
      : set_stretch_amount_count_(0),
        request_animate_count_(0),
        pinned_neg_x_(false),
        pinned_pos_x_(false),
        pinned_neg_y_(false),
        pinned_pos_y_(false) {}
  virtual ~MockScrollElasticityHelper() {}

  // cc::ScrollElasticityHelper implementation:
  gfx::Vector2dF StretchAmount() override { return stretch_amount_; }
  void SetStretchAmount(const gfx::Vector2dF& stretch_amount) override {
    set_stretch_amount_count_ += 1;
    stretch_amount_ = stretch_amount;
  }
  bool PinnedInDirection(const gfx::Vector2dF& direction) override {
    if (pinned_neg_x_ && direction.x() < 0)
      return true;
    if (pinned_pos_x_ && direction.x() > 0)
      return true;
    if (pinned_neg_y_ && direction.y() < 0)
      return true;
    if (pinned_pos_y_ && direction.y() > 0)
      return true;
    return false;
  }
  bool CanScrollHorizontally() override { return true; }
  bool CanScrollVertically() override { return true; }
  void RequestAnimate() override { request_animate_count_ += 1; }

  // Counters for number of times functions were called.
  int request_animate_count() const { return request_animate_count_; }
  int set_stretch_amount_count() const { return set_stretch_amount_count_; }
  void SetPinned(bool neg_x, bool pos_x, bool neg_y, bool pos_y) {
    pinned_neg_x_ = neg_x;
    pinned_pos_x_ = pos_x;
    pinned_neg_y_ = neg_y;
    pinned_pos_y_ = pos_y;
  }

 private:
  gfx::Vector2dF stretch_amount_;
  int set_stretch_amount_count_;
  int request_animate_count_;
  bool pinned_neg_x_;
  bool pinned_pos_x_;
  bool pinned_neg_y_;
  bool pinned_pos_y_;
};

class ScrollElasticityControllerTest : public testing::Test {
 public:
  ScrollElasticityControllerTest()
      : controller_(&helper_),
        input_event_count_(0),
        current_time_(base::TimeTicks::FromInternalValue(100000000ull)) {}
  virtual ~ScrollElasticityControllerTest() {}

  void SendMouseWheelEvent(
      Phase phase,
      Phase momentum_phase,
      const gfx::Vector2dF& event_delta = gfx::Vector2dF(),
      const gfx::Vector2dF& overscroll_delta = gfx::Vector2dF()) {
    blink::WebMouseWheelEvent event;
    event.phase = static_cast<blink::WebMouseWheelEvent::Phase>(phase);
    event.momentumPhase =
        static_cast<blink::WebMouseWheelEvent::Phase>(momentum_phase);
    event.deltaX = -event_delta.x();
    event.deltaY = -event_delta.y();
    TickCurrentTime();
    event.timeStampSeconds = (current_time_ - base::TimeTicks()).InSecondsF();

    cc::InputHandlerScrollResult scroll_result;
    scroll_result.did_overscroll_root = !overscroll_delta.IsZero();
    scroll_result.unused_scroll_delta = overscroll_delta;

    controller_.ObserveWheelEventAndResult(event, scroll_result);
    input_event_count_ += 1;
  }

  const base::TimeTicks& TickCurrentTime() {
    current_time_ += base::TimeDelta::FromSecondsD(1 / 60.f);
    return current_time_;
  }
  void TickCurrentTimeAndAnimate() {
    TickCurrentTime();
    controller_.Animate(current_time_);
  }

  MockScrollElasticityHelper helper_;
  InputScrollElasticityController controller_;
  int input_event_count_;
  base::TimeTicks current_time_;
};

// Verify that stretching only occurs in one axis at a time, and that it
// is biased to the Y axis.
TEST_F(ScrollElasticityControllerTest, Axis) {
  helper_.SetPinned(true, true, true, true);

  // If we push equally in the X and Y directions, we should see a stretch only
  // in the Y direction.
  SendMouseWheelEvent(PhaseBegan, PhaseNone);
  SendMouseWheelEvent(PhaseChanged, PhaseNone, gfx::Vector2dF(10, 10),
                      gfx::Vector2dF(10, 10));
  EXPECT_EQ(1, helper_.set_stretch_amount_count());
  EXPECT_EQ(0.f, helper_.StretchAmount().x());
  EXPECT_LT(0.f, helper_.StretchAmount().y());
  helper_.SetStretchAmount(gfx::Vector2dF());
  EXPECT_EQ(2, helper_.set_stretch_amount_count());
  SendMouseWheelEvent(PhaseEnded, PhaseNone);
  EXPECT_EQ(0, helper_.request_animate_count());

  // If we push more in the X direction than the Y direction, we should see a
  // stretch only in the X direction. This decision should be based on the
  // input delta, not the actual overscroll delta.
  SendMouseWheelEvent(PhaseBegan, PhaseNone);
  SendMouseWheelEvent(PhaseChanged, PhaseNone, gfx::Vector2dF(-25, 10),
                      gfx::Vector2dF(-25, 40));
  EXPECT_EQ(3, helper_.set_stretch_amount_count());
  EXPECT_GT(0.f, helper_.StretchAmount().x());
  EXPECT_EQ(0.f, helper_.StretchAmount().y());
  helper_.SetStretchAmount(gfx::Vector2dF());
  EXPECT_EQ(4, helper_.set_stretch_amount_count());
  SendMouseWheelEvent(PhaseEnded, PhaseNone);
  EXPECT_EQ(0, helper_.request_animate_count());
}

// Verify that we need a total overscroll delta of at least 10 in a pinned
// direction before we start stretching.
TEST_F(ScrollElasticityControllerTest, MinimumDeltaBeforeStretch) {
  // We should not start stretching while we are not pinned in the direction
  // of the scroll (even if there is an overscroll delta). We have to wait for
  // the regular scroll to eat all of the events.
  helper_.SetPinned(false, false, false, false);
  SendMouseWheelEvent(PhaseMayBegin, PhaseNone);
  SendMouseWheelEvent(PhaseBegan, PhaseNone);
  SendMouseWheelEvent(PhaseChanged, PhaseNone, gfx::Vector2dF(0, 10),
                      gfx::Vector2dF(0, 10));
  SendMouseWheelEvent(PhaseChanged, PhaseNone, gfx::Vector2dF(0, 10),
                      gfx::Vector2dF(0, 10));
  EXPECT_EQ(0, helper_.set_stretch_amount_count());

  // Now pin the -X and +Y direction. The first event will not generate a
  // stretch
  // because it is below the delta threshold of 10.
  helper_.SetPinned(true, false, false, true);
  SendMouseWheelEvent(PhaseChanged, PhaseNone, gfx::Vector2dF(0, 10),
                      gfx::Vector2dF(0, 8));
  EXPECT_EQ(0, helper_.set_stretch_amount_count());

  // Make the next scroll be in the -X direction more than the +Y direction,
  // which will erase the memory of the previous unused delta of 8.
  SendMouseWheelEvent(PhaseChanged, PhaseNone, gfx::Vector2dF(-10, 5),
                      gfx::Vector2dF(-8, 10));
  EXPECT_EQ(0, helper_.set_stretch_amount_count());

  // Now push against the pinned +Y direction again by 8. We reset the
  // previous delta, so this will not generate a stretch.
  SendMouseWheelEvent(PhaseChanged, PhaseNone, gfx::Vector2dF(0, 10),
                      gfx::Vector2dF(0, 8));
  EXPECT_EQ(0, helper_.set_stretch_amount_count());

  // Push against +Y by another 8. This gets us above the delta threshold of
  // 10, so we should now have had the stretch set, and it should be in the
  // +Y direction. The scroll in the -X direction should have been forgotten.
  SendMouseWheelEvent(PhaseChanged, PhaseNone, gfx::Vector2dF(0, 10),
                      gfx::Vector2dF(0, 8));
  EXPECT_EQ(1, helper_.set_stretch_amount_count());
  EXPECT_EQ(0.f, helper_.StretchAmount().x());
  EXPECT_LT(0.f, helper_.StretchAmount().y());

  // End the gesture. Because there is a non-zero stretch, we should be in the
  // animated state, and should have had a frame requested.
  EXPECT_EQ(0, helper_.request_animate_count());
  SendMouseWheelEvent(PhaseEnded, PhaseNone);
  EXPECT_EQ(1, helper_.request_animate_count());
}

// Verify that an stretch caused by a momentum scroll will switch to the
// animating mode, where input events are ignored, and the stretch is updated
// while animating.
TEST_F(ScrollElasticityControllerTest, MomentumAnimate) {
  // Do an active scroll, then switch to the momentum phase and scroll for a
  // bit.
  helper_.SetPinned(false, false, false, false);
  SendMouseWheelEvent(PhaseBegan, PhaseNone);
  SendMouseWheelEvent(PhaseChanged, PhaseNone, gfx::Vector2dF(0, -80),
                      gfx::Vector2dF(0, 0));
  SendMouseWheelEvent(PhaseChanged, PhaseNone, gfx::Vector2dF(0, -80),
                      gfx::Vector2dF(0, 0));
  SendMouseWheelEvent(PhaseChanged, PhaseNone, gfx::Vector2dF(0, -80),
                      gfx::Vector2dF(0, 0));
  SendMouseWheelEvent(PhaseEnded, PhaseNone);
  SendMouseWheelEvent(PhaseNone, PhaseBegan);
  SendMouseWheelEvent(PhaseNone, PhaseChanged, gfx::Vector2dF(0, -80),
                      gfx::Vector2dF(0, 0));
  SendMouseWheelEvent(PhaseNone, PhaseChanged, gfx::Vector2dF(0, -80),
                      gfx::Vector2dF(0, 0));
  SendMouseWheelEvent(PhaseNone, PhaseChanged, gfx::Vector2dF(0, -80),
                      gfx::Vector2dF(0, 0));
  EXPECT_EQ(0, helper_.set_stretch_amount_count());

  // Hit the -Y edge and overscroll slightly, but not enough to go over the
  // threshold to cause a stretch.
  helper_.SetPinned(false, false, true, false);
  SendMouseWheelEvent(PhaseNone, PhaseChanged, gfx::Vector2dF(0, -80),
                      gfx::Vector2dF(0, -8));
  EXPECT_EQ(0, helper_.set_stretch_amount_count());
  EXPECT_EQ(0, helper_.request_animate_count());

  // Take another step, this time going over the threshold. This should update
  // the stretch amount, and then switch to the animating mode.
  SendMouseWheelEvent(PhaseNone, PhaseChanged, gfx::Vector2dF(0, -80),
                      gfx::Vector2dF(0, -80));
  EXPECT_EQ(1, helper_.set_stretch_amount_count());
  EXPECT_EQ(1, helper_.request_animate_count());
  EXPECT_GT(-1.f, helper_.StretchAmount().y());

  // Subsequent momentum events should do nothing.
  SendMouseWheelEvent(PhaseNone, PhaseChanged, gfx::Vector2dF(0, -80),
                      gfx::Vector2dF(0, -80));
  SendMouseWheelEvent(PhaseNone, PhaseChanged, gfx::Vector2dF(0, -80),
                      gfx::Vector2dF(0, -80));
  SendMouseWheelEvent(PhaseNone, PhaseEnded, gfx::Vector2dF(0, -80),
                      gfx::Vector2dF(0, -80));
  EXPECT_EQ(1, helper_.set_stretch_amount_count());
  EXPECT_EQ(1, helper_.request_animate_count());

  // Subsequent animate events should update the stretch amount and request
  // another frame.
  TickCurrentTimeAndAnimate();
  EXPECT_EQ(2, helper_.set_stretch_amount_count());
  EXPECT_EQ(2, helper_.request_animate_count());
  EXPECT_GT(-1.f, helper_.StretchAmount().y());

  // Touching the trackpad (a PhaseMayBegin event) should disable animation.
  SendMouseWheelEvent(PhaseMayBegin, PhaseNone);
  TickCurrentTimeAndAnimate();
  EXPECT_EQ(2, helper_.set_stretch_amount_count());
  EXPECT_EQ(2, helper_.request_animate_count());

  // Releasing the trackpad should re-enable animation.
  SendMouseWheelEvent(PhaseCancelled, PhaseNone);
  EXPECT_EQ(2, helper_.set_stretch_amount_count());
  EXPECT_EQ(3, helper_.request_animate_count());
  TickCurrentTimeAndAnimate();
  EXPECT_EQ(3, helper_.set_stretch_amount_count());
  EXPECT_EQ(4, helper_.request_animate_count());

  // Keep animating frames until the stretch returns to rest.
  int stretch_count = 3;
  int animate_count = 4;
  while (1) {
    TickCurrentTimeAndAnimate();
    if (helper_.StretchAmount().IsZero()) {
      stretch_count += 1;
      EXPECT_EQ(stretch_count, helper_.set_stretch_amount_count());
      EXPECT_EQ(animate_count, helper_.request_animate_count());
      break;
    }
    stretch_count += 1;
    animate_count += 1;
    EXPECT_EQ(stretch_count, helper_.set_stretch_amount_count());
    EXPECT_EQ(animate_count, helper_.request_animate_count());
  }

  // After coming to rest, no subsequent animate calls change anything.
  TickCurrentTimeAndAnimate();
  EXPECT_EQ(stretch_count, helper_.set_stretch_amount_count());
  EXPECT_EQ(animate_count, helper_.request_animate_count());
}

}  // namespace
}  // namespace content
