// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/scheduler/child/scheduler_helper.h"

#include "base/callback.h"
#include "base/test/simple_test_tick_clock.h"
#include "cc/test/ordered_simple_task_runner.h"
#include "components/scheduler/child/scheduler_task_runner_delegate_for_test.h"
#include "components/scheduler/child/task_queue.h"
#include "components/scheduler/child/test_time_source.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;
using testing::AnyNumber;
using testing::Invoke;
using testing::Return;

namespace scheduler {

namespace {
void AppendToVectorTestTask(std::vector<std::string>* vector,
                            std::string value) {
  vector->push_back(value);
}

void AppendToVectorReentrantTask(base::SingleThreadTaskRunner* task_runner,
                                 std::vector<int>* vector,
                                 int* reentrant_count,
                                 int max_reentrant_count) {
  vector->push_back((*reentrant_count)++);
  if (*reentrant_count < max_reentrant_count) {
    task_runner->PostTask(
        FROM_HERE,
        base::Bind(AppendToVectorReentrantTask, base::Unretained(task_runner),
                   vector, reentrant_count, max_reentrant_count));
  }
}

};  // namespace

class SchedulerHelperTest : public testing::Test {
 public:
  SchedulerHelperTest()
      : clock_(new base::SimpleTestTickClock()),
        mock_task_runner_(new cc::OrderedSimpleTaskRunner(clock_.get(), false)),
        main_task_runner_(
            SchedulerTaskRunnerDelegateForTest::Create(mock_task_runner_)),
        scheduler_helper_(new SchedulerHelper(
            main_task_runner_,
            "test.scheduler",
            TRACE_DISABLED_BY_DEFAULT("test.scheduler"),
            TRACE_DISABLED_BY_DEFAULT("test.scheduler.dbg"))),
        default_task_runner_(scheduler_helper_->DefaultTaskRunner()) {
    clock_->Advance(base::TimeDelta::FromMicroseconds(5000));
    scheduler_helper_->SetTimeSourceForTesting(
        make_scoped_ptr(new TestTimeSource(clock_.get())));
    scheduler_helper_->GetTaskQueueManagerForTesting()->SetTimeSourceForTesting(
        make_scoped_ptr(new TestTimeSource(clock_.get())));
  }

  ~SchedulerHelperTest() override {}

  void TearDown() override {
    // Check that all tests stop posting tasks.
    mock_task_runner_->SetAutoAdvanceNowToPendingTasks(true);
    while (mock_task_runner_->RunUntilIdle()) {
    }
  }

  void RunUntilIdle() { mock_task_runner_->RunUntilIdle(); }

  template <typename E>
  static void CallForEachEnumValue(E first,
                                   E last,
                                   const char* (*function)(E)) {
    for (E val = first; val < last;
         val = static_cast<E>(static_cast<int>(val) + 1)) {
      (*function)(val);
    }
  }

 protected:
  scoped_ptr<base::SimpleTestTickClock> clock_;
  scoped_refptr<cc::OrderedSimpleTaskRunner> mock_task_runner_;

  scoped_refptr<SchedulerTaskRunnerDelegateForTest> main_task_runner_;
  scoped_ptr<SchedulerHelper> scheduler_helper_;
  scoped_refptr<base::SingleThreadTaskRunner> default_task_runner_;

  DISALLOW_COPY_AND_ASSIGN(SchedulerHelperTest);
};

TEST_F(SchedulerHelperTest, TestPostDefaultTask) {
  std::vector<std::string> run_order;
  default_task_runner_->PostTask(
      FROM_HERE, base::Bind(&AppendToVectorTestTask, &run_order, "D1"));
  default_task_runner_->PostTask(
      FROM_HERE, base::Bind(&AppendToVectorTestTask, &run_order, "D2"));
  default_task_runner_->PostTask(
      FROM_HERE, base::Bind(&AppendToVectorTestTask, &run_order, "D3"));
  default_task_runner_->PostTask(
      FROM_HERE, base::Bind(&AppendToVectorTestTask, &run_order, "D4"));

  RunUntilIdle();
  EXPECT_THAT(run_order,
              testing::ElementsAre(std::string("D1"), std::string("D2"),
                                   std::string("D3"), std::string("D4")));
}

TEST_F(SchedulerHelperTest, TestRentrantTask) {
  int count = 0;
  std::vector<int> run_order;
  default_task_runner_->PostTask(
      FROM_HERE, base::Bind(AppendToVectorReentrantTask, default_task_runner_,
                            &run_order, &count, 5));
  RunUntilIdle();

  EXPECT_THAT(run_order, testing::ElementsAre(0, 1, 2, 3, 4));
}

TEST_F(SchedulerHelperTest, IsShutdown) {
  EXPECT_FALSE(scheduler_helper_->IsShutdown());

  scheduler_helper_->Shutdown();
  EXPECT_TRUE(scheduler_helper_->IsShutdown());
}

TEST_F(SchedulerHelperTest, DefaultTaskRunnerRegistration) {
  EXPECT_EQ(main_task_runner_->default_task_runner(),
            scheduler_helper_->DefaultTaskRunner());
  scheduler_helper_->Shutdown();
  EXPECT_EQ(nullptr, main_task_runner_->default_task_runner());
}

namespace {
class MockTaskObserver : public base::MessageLoop::TaskObserver {
 public:
  MOCK_METHOD1(DidProcessTask, void(const base::PendingTask& task));
  MOCK_METHOD1(WillProcessTask, void(const base::PendingTask& task));
};

void NopTask() {}
} // namespace

TEST_F(SchedulerHelperTest, ObserversNotifiedFor_DefaultTaskRunner) {
  MockTaskObserver observer;
  scheduler_helper_->AddTaskObserver(&observer);

  scheduler_helper_->DefaultTaskRunner()->PostTask(FROM_HERE,
                                                   base::Bind(&NopTask));

  EXPECT_CALL(observer, WillProcessTask(_)).Times(1);
  EXPECT_CALL(observer, DidProcessTask(_)).Times(1);
  RunUntilIdle();
}

TEST_F(SchedulerHelperTest, ObserversNotNotifiedFor_ControlTaskRunner) {
  MockTaskObserver observer;
  scheduler_helper_->AddTaskObserver(&observer);

  scheduler_helper_->ControlTaskRunner()->PostTask(FROM_HERE,
                                                   base::Bind(&NopTask));

  EXPECT_CALL(observer, WillProcessTask(_)).Times(0);
  EXPECT_CALL(observer, DidProcessTask(_)).Times(0);
  RunUntilIdle();
}

TEST_F(SchedulerHelperTest,
       ObserversNotNotifiedFor_ControlAfterWakeUpTaskRunner) {
  MockTaskObserver observer;
  scheduler_helper_->AddTaskObserver(&observer);

  scheduler_helper_->ControlAfterWakeUpTaskRunner()->PostTask(
      FROM_HERE, base::Bind(&NopTask));

  EXPECT_CALL(observer, WillProcessTask(_)).Times(0);
  EXPECT_CALL(observer, DidProcessTask(_)).Times(0);
  scheduler_helper_->ControlAfterWakeUpTaskRunner()->PumpQueue();
  RunUntilIdle();
}

}  // namespace scheduler
