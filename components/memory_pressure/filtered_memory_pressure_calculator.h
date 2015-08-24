// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_MEMORY_PRESSURE_FILTERED_MEMORY_PRESSURE_CALCULATOR_H_
#define COMPONENTS_MEMORY_PRESSURE_FILTERED_MEMORY_PRESSURE_CALCULATOR_H_

#include "base/time/tick_clock.h"
#include "components/memory_pressure/memory_pressure_calculator.h"

namespace memory_pressure {

#if defined(MEMORY_PRESSURE_IS_POLLING)

// A utility class that provides rate-limiting and hysteresis on raw memory
// pressure calculations. This is identical across all platforms, but only used
// on those that do not have native memory pressure signals.
class FilteredMemoryPressureCalculator : public MemoryPressureCalculator {
 public:
  // The minimum time that must pass between successive polls. This enforces an
  // upper bound on the rate of calls to the contained MemoryPressureCalculator.
  static const int kMinimumTimeBetweenSamplesMs;

  // The cooldown period when transitioning from critical to moderate/no memory
  // pressure, or from moderate to none.
  static const int kCriticalPressureCooldownPeriodMs;
  static const int kModeratePressureCooldownPeriodMs;

  explicit FilteredMemoryPressureCalculator(
      scoped_ptr<MemoryPressureCalculator> pressure_calculator);
  ~FilteredMemoryPressureCalculator() override;

  // Calculates the current pressure level.
  MemoryPressureLevel CalculateCurrentPressureLevel() override;

  // Testing seam for configuring the tick clock in use.
  void set_tick_clock(scoped_ptr<base::TickClock> tick_clock) {
    tick_clock_ = tick_clock.Pass();
  }

  // Accessors for unittesting.
  bool cooldown_in_progress() const { return cooldown_in_progress_; }
  base::TimeTicks cooldown_start_time() const { return cooldown_start_time_; }
  MemoryPressureLevel cooldown_high_tide() const { return cooldown_high_tide_; }

 private:
  friend class TestFilteredMemoryPressureCalculator;

  // The delegate tick clock. This is settable as a testing seam.
  scoped_ptr<base::TickClock> tick_clock_;

  // The delegate pressure calculator. Provided by the constructor.
  scoped_ptr<MemoryPressureCalculator> pressure_calculator_;

  // The memory pressure currently being reported.
  MemoryPressureLevel current_pressure_level_;

  // The last time a sample was taken.
  bool samples_taken_;
  base::TimeTicks last_sample_time_;

  // State of an ongoing cooldown period, if any. The high-tide line indicates
  // the highest memory pressure level (*below* the current one) that was
  // encountered during the cooldown period. This allows a cooldown to
  // transition directly from critical to none if *no* moderate pressure signals
  // were seen during the period, otherwise it forces it to pass through a
  // moderate cooldown as well.
  bool cooldown_in_progress_;
  base::TimeTicks cooldown_start_time_;
  MemoryPressureLevel cooldown_high_tide_;

  DISALLOW_COPY_AND_ASSIGN(FilteredMemoryPressureCalculator);
};

#endif  // defined(MEMORY_PRESSURE_IS_POLLING)

}  // namespace memory_pressure

#endif  // COMPONENTS_MEMORY_PRESSURE_FILTERED_MEMORY_PRESSURE_CALCULATOR_H_
