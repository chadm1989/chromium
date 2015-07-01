// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gl/gpu_timing.h"

#include "base/time/time.h"
#include "ui/gl/gl_bindings.h"
#include "ui/gl/gl_context.h"
#include "ui/gl/gl_version_info.h"

namespace gfx {

int64_t NanoToMicro(uint64_t nano_seconds) {
  const uint64_t up = nano_seconds + base::Time::kNanosecondsPerMicrosecond / 2;
  return static_cast<int64_t>(up / base::Time::kNanosecondsPerMicrosecond);
}

class GPUTimingImpl : public GPUTiming {
 public:
   GPUTimingImpl(GLContextReal* context);
  ~GPUTimingImpl() override;

  void ForceTimeElapsedQuery() { force_time_elapsed_query_ = true; }
  bool IsForceTimeElapsedQuery() { return force_time_elapsed_query_; }

  GPUTiming::TimerType GetTimerType() const { return timer_type_; }

  uint32_t GetDisjointCount();
  int64 CalculateTimerOffset();

  scoped_refptr<QueryResult> BeginElapsedTimeQuery();
  void EndElapsedTimeQuery(scoped_refptr<QueryResult> result);

  scoped_refptr<QueryResult> DoTimeStampQuery();

  int64 GetCurrentCPUTime() {
    return cpu_time_for_testing_.is_null()
           ? (base::TraceTicks::Now() - base::TraceTicks()).InMicroseconds()
           : cpu_time_for_testing_.Run();
  }
  void SetCpuTimeForTesting(const base::Callback<int64(void)>& cpu_time) {
    cpu_time_for_testing_ = cpu_time;
  }

  void UpdateQueryResults();

  int64_t GetMaxTimeStamp() { return max_time_stamp_; }
  void UpdateMaxTimeStamp(int64_t value) {
    max_time_stamp_ = std::max(max_time_stamp_, value);
  }

  uint32_t GetElapsedQueryCount() { return elapsed_query_count_; }
  void IncElapsedQueryCount() { elapsed_query_count_++; }
  void DecElapsedQueryCount() { elapsed_query_count_--; }

  void SetLastElapsedQuery(scoped_refptr<TimeElapsedTimerQuery> query);
  scoped_refptr<TimeElapsedTimerQuery> GetLastElapsedQuery();

  void HandleBadQuery();
  bool IsGoodQueryID(uint32_t query_id);

 private:
  scoped_refptr<GPUTimingClient> CreateGPUTimingClient() override;

  base::Callback<int64(void)> cpu_time_for_testing_;
  GPUTiming::TimerType timer_type_ = GPUTiming::kTimerTypeInvalid;
  uint32_t disjoint_counter_ = 0;
  int64 offset_ = 0;  // offset cache when timer_type_ == kTimerTypeARB
  bool offset_valid_ = false;
  bool force_time_elapsed_query_ = false;

  uint32_t next_timer_query_id_ = 0;
  uint32_t next_good_timer_query_id_ = 0; // identify bad ids for disjoints.
  uint32_t query_disjoint_count_ = 0;

  // Extra state tracking data for elapsed timer queries.
  int64_t max_time_stamp_ = 0;
  uint32_t elapsed_query_count_ = 0;
  scoped_refptr<TimeElapsedTimerQuery> last_elapsed_query_;

  std::deque<scoped_refptr<TimerQuery> > queries_;

  DISALLOW_COPY_AND_ASSIGN(GPUTimingImpl);
};

class QueryResult : public base::RefCounted<QueryResult> {
 public:
  QueryResult() {}

  bool IsAvailable() const { return available_; }
  int64_t GetDelta() const { return end_value_ - start_value_; }
  int64_t GetStartValue() const { return start_value_; }
  int64_t GetEndValue() const { return end_value_; }

  void SetStartValue(int64_t value) { start_value_ = value; }
  void SetEndValue(int64_t value) { available_ = true; end_value_ = value; }

 private:
  friend class base::RefCounted<QueryResult>;
  ~QueryResult() {}

  bool available_ = false;
  int64_t start_value_ = 0;
  int64_t end_value_ = 0;

  DISALLOW_COPY_AND_ASSIGN(QueryResult);
};

class TimerQuery : public base::RefCounted<TimerQuery> {
 public:
  TimerQuery(uint32_t next_id);
  virtual void Destroy() = 0;

  // Returns true when UpdateQueryResults() is ready to be called.
  virtual bool IsAvailable(GPUTimingImpl* gpu_timing) = 0;

  // Fills out query result start and end, called after IsAvailable() is true.
  virtual void UpdateQueryResults(GPUTimingImpl* gpu_timing) = 0;

  // Called when Query is next in line, used to transition states.
  virtual void PrepareNextUpdate(scoped_refptr<TimerQuery> prev) {}

  uint32_t timer_query_id_ = 0;
  int64_t time_stamp_ = 0; // Timestamp of the query, could be estimated.

 protected:
  friend class base::RefCounted<TimerQuery>;
  virtual ~TimerQuery();
  DISALLOW_COPY_AND_ASSIGN(TimerQuery);
};

TimerQuery::TimerQuery(uint32_t next_id)
    : timer_query_id_(next_id) {
}

TimerQuery::~TimerQuery() {
}

class TimeElapsedTimerQuery : public TimerQuery {
 public:
  TimeElapsedTimerQuery(GPUTimingImpl* gpu_timing, uint32_t next_id)
      : TimerQuery(next_id) {
    glGenQueries(1, &gl_query_id_);
  }

  void Destroy() override {
    glDeleteQueries(1, &gl_query_id_);
  }

  scoped_refptr<QueryResult> StartQuery(GPUTimingImpl* gpu_timing) {
    DCHECK(query_result_start_.get() == nullptr);
    query_begin_cpu_time_ = gpu_timing->GetCurrentCPUTime();
    if (gpu_timing->GetElapsedQueryCount() == 0) {
      first_top_level_query_ = true;
    } else {
      // Stop the current timer query.
      glEndQuery(GL_TIME_ELAPSED);
    }

    // begin a new one time elapsed query.
    glBeginQuery(GL_TIME_ELAPSED, gl_query_id_);
    query_result_start_ = new QueryResult();

    // Update GPUTiming state.
    gpu_timing->SetLastElapsedQuery(this);
    gpu_timing->IncElapsedQueryCount();

    return query_result_start_;
  }

  void EndQuery(GPUTimingImpl* gpu_timing,
                scoped_refptr<QueryResult> result) {
    DCHECK(gpu_timing->GetElapsedQueryCount() != 0);

    scoped_refptr<TimeElapsedTimerQuery> last_query =
        gpu_timing->GetLastElapsedQuery();
    DCHECK(last_query.get());
    DCHECK(last_query->query_result_end_.get() == nullptr);

    last_query->query_result_end_ = result;
    gpu_timing->DecElapsedQueryCount();

    if (gpu_timing->GetElapsedQueryCount() != 0) {
      // Continue timer if there are still ongoing queries.
      glEndQuery(GL_TIME_ELAPSED);
      glBeginQuery(GL_TIME_ELAPSED, gl_query_id_);
      gpu_timing->SetLastElapsedQuery(this);
    } else {
      // Simply end the query and reset the current offset
      glEndQuery(GL_TIME_ELAPSED);
      gpu_timing->SetLastElapsedQuery(nullptr);
    }
  }

  // Returns true when UpdateQueryResults() is ready to be called.
  bool IsAvailable(GPUTimingImpl* gpu_timing) override {
    if (gpu_timing->GetElapsedQueryCount() != 0 &&
        gpu_timing->GetLastElapsedQuery() == this) {
      // Cannot query if result is available if EndQuery has not been called.
      // Since only one query is going on at a time, the end query is only not
      // called for the very last query when ongoing query counter is not 0.
      return false;
    }

    GLint done = 0;
    glGetQueryObjectiv(gl_query_id_, GL_QUERY_RESULT_AVAILABLE, &done);
    return !!done;
  }

  // Fills out query result start and end, called after IsAvailable() is true.
  void UpdateQueryResults(GPUTimingImpl* gpu_timing) override {
    DCHECK(IsAvailable(gpu_timing));

    GLuint64 result_value = 0;
    glGetQueryObjectui64v(gl_query_id_, GL_QUERY_RESULT, &result_value);
    const int64_t micro_results = NanoToMicro(result_value);

    // Adjust prev query end time if it is before the current max.
    const int64_t start_time =
        std::max(first_top_level_query_ ? query_begin_cpu_time_ : 0,
                 std::max(prev_query_end_time_,
                          gpu_timing->GetMaxTimeStamp()));

    // As a sanity check, is result value is greater than the time allotted we
    // can safely say this is garbage data
    const int64_t max_possible_time =
        gpu_timing->GetCurrentCPUTime() - query_begin_cpu_time_;
    if (micro_results > max_possible_time) {
      gpu_timing->HandleBadQuery();
    }

    // Elapsed queries need to be adjusted so they are relative to one another.
    // Absolute timer queries are already relative to one another absolutely.
    time_stamp_ = start_time + micro_results;

    if (query_result_start_.get()) {
      query_result_start_->SetStartValue(start_time);
    }
    if (query_result_end_.get()) {
      query_result_end_->SetEndValue(time_stamp_);
    }
  }

  // Called when Query is next in line, used to transition states.
  void PrepareNextUpdate(scoped_refptr<TimerQuery> prev) override {
    prev_query_end_time_ = prev->time_stamp_;
  }

 private:
  ~TimeElapsedTimerQuery() override {}

  bool first_top_level_query_ = false;
  uint32_t gl_query_id_ = 0;
  int64_t prev_query_end_time_ = 0;
  int64_t query_begin_cpu_time_ = 0;
  scoped_refptr<QueryResult> query_result_start_;
  scoped_refptr<QueryResult> query_result_end_;
};

class TimeStampTimerQuery : public TimerQuery {
 public:
  TimeStampTimerQuery(uint32_t next_id)
      : TimerQuery(next_id) {
    glGenQueries(1, &gl_query_id_);
  }

  void Destroy() override {
    glDeleteQueries(1, &gl_query_id_);
  }

  scoped_refptr<QueryResult> DoQuery() {
    glQueryCounter(gl_query_id_, GL_TIMESTAMP);
    query_result_ = new QueryResult();
    return query_result_;
  }

  // Returns true when UpdateQueryResults() is ready to be called.
  bool IsAvailable(GPUTimingImpl* gpu_timing) override {
    GLint done = 0;
    glGetQueryObjectiv(gl_query_id_, GL_QUERY_RESULT_AVAILABLE, &done);
    return !!done;
  }

  // Fills out query result start and end, called after IsAvailable() is true.
  void UpdateQueryResults(GPUTimingImpl* gpu_timing) override {
    DCHECK(IsAvailable(gpu_timing));

    GLuint64 result_value = 0;
    glGetQueryObjectui64v(gl_query_id_, GL_QUERY_RESULT, &result_value);
    const int64_t micro_results = NanoToMicro(result_value);

    const int64 offset = gpu_timing->CalculateTimerOffset();
    const int64_t adjusted_result = micro_results + offset;
    DCHECK(query_result_.get());
    query_result_->SetStartValue(adjusted_result);
    query_result_->SetEndValue(adjusted_result);
    time_stamp_ = adjusted_result;
  }

 private:
  ~TimeStampTimerQuery() override {}
  uint32_t gl_query_id_ = 0;
  scoped_refptr<QueryResult> query_result_;
};

GPUTimingImpl::GPUTimingImpl(GLContextReal* context) {
  DCHECK(context);
  const GLVersionInfo* version_info = context->GetVersionInfo();
  DCHECK(version_info);
  if (version_info->is_es3 &&  // glGetInteger64v is supported under ES3.
    context->HasExtension("GL_EXT_disjoint_timer_query")) {
    timer_type_ = GPUTiming::kTimerTypeDisjoint;
  } else if (context->HasExtension("GL_ARB_timer_query")) {
    timer_type_ = GPUTiming::kTimerTypeARB;
  } else if (context->HasExtension("GL_EXT_timer_query")) {
    timer_type_ = GPUTiming::kTimerTypeEXT;
  }
}

GPUTimingImpl::~GPUTimingImpl() {
}

uint32_t GPUTimingImpl::GetDisjointCount() {
  if (timer_type_ == GPUTiming::kTimerTypeDisjoint) {
    GLint disjoint_value = 0;
    glGetIntegerv(GL_GPU_DISJOINT_EXT, &disjoint_value);
    if (disjoint_value) {
      offset_valid_ = false;
      disjoint_counter_++;
    }
  }
  return disjoint_counter_;
}

int64 GPUTimingImpl::CalculateTimerOffset() {
  if (!offset_valid_) {
    if (timer_type_ == GPUTiming::kTimerTypeDisjoint ||
        timer_type_ == GPUTiming::kTimerTypeARB) {
      GLint64 gl_now = 0;
      glGetInteger64v(GL_TIMESTAMP, &gl_now);
      int64_t micro_now = NanoToMicro(gl_now);
      offset_ = GetCurrentCPUTime() - micro_now;
      offset_valid_ = (timer_type_ == GPUTiming::kTimerTypeARB);
    } else {
      offset_ = 0;
      offset_valid_ = true;
    }
  }
  return offset_;
}

scoped_refptr<QueryResult> GPUTimingImpl::BeginElapsedTimeQuery() {
  DCHECK(timer_type_ != GPUTiming::kTimerTypeInvalid);

  queries_.push_back(new TimeElapsedTimerQuery(this, next_timer_query_id_++));
  return static_cast<TimeElapsedTimerQuery*>(
      queries_.back().get())->StartQuery(this);
}

void GPUTimingImpl::EndElapsedTimeQuery(scoped_refptr<QueryResult> result) {
  DCHECK(timer_type_ != GPUTiming::kTimerTypeInvalid);
  DCHECK(result.get());

  if (GetElapsedQueryCount() > 1) {
    // Create new elapsed timer query if there are still ongoing queries.
    queries_.push_back(new TimeElapsedTimerQuery(this,
                                                 next_timer_query_id_++));
    static_cast<TimeElapsedTimerQuery*>(
        queries_.back().get())->EndQuery(this, result);
  } else {
    // Simply end the query and reset the current offset
    DCHECK(GetLastElapsedQuery().get());
    GetLastElapsedQuery()->EndQuery(this, result);
    DCHECK(GetLastElapsedQuery().get() == nullptr);
  }
}

scoped_refptr<QueryResult> GPUTimingImpl::DoTimeStampQuery() {
  DCHECK(timer_type_ == GPUTiming::kTimerTypeDisjoint ||
         timer_type_ == GPUTiming::kTimerTypeARB);

  if (force_time_elapsed_query_) {
    // Replace with elapsed timer queries instead.
    scoped_refptr<QueryResult> result = BeginElapsedTimeQuery();
    EndElapsedTimeQuery(result);
    return result;
  }

  queries_.push_back(new TimeStampTimerQuery(next_timer_query_id_++));
  return static_cast<TimeStampTimerQuery*>(queries_.back().get())->DoQuery();
}

void GPUTimingImpl::UpdateQueryResults() {
  // Query availability of and count the queries that are available.
  int available_queries = 0;
  for (const scoped_refptr<TimerQuery>& query : queries_) {
    if (!query->IsAvailable(this))
      break;
    available_queries++;
  }

  // Check for disjoints, this must be done after we checked for availability.
  const uint32_t disjoint_counter = GetDisjointCount();
  if (disjoint_counter != query_disjoint_count_) {
    next_good_timer_query_id_ = next_timer_query_id_;
    query_disjoint_count_ = disjoint_counter;
  }

  // Fill in the query result data once we know the disjoint value is updated.
  // Note that even if disjoint happened and the values may or may not be
  // garbage, we still fill it in and let GPUTimingClient's detect and disgard
  // bad query data. The only thing we need to account for here is to not
  // use garbade timer data to fill states such as max query times.
  for (int i = 0; i < available_queries; ++i) {
    scoped_refptr<TimerQuery> query = queries_.front();

    query->UpdateQueryResults(this);
    DCHECK(query->time_stamp_) << "Query Timestamp was not updated.";

    // For good queries, keep track of the max valid time stamps.
    if (IsGoodQueryID(query->timer_query_id_))
      UpdateMaxTimeStamp(query->time_stamp_);

    query->Destroy();
    queries_.pop_front();

    if (!queries_.empty())
      queries_.front()->PrepareNextUpdate(query);
  }
}

void GPUTimingImpl::SetLastElapsedQuery(
    scoped_refptr<TimeElapsedTimerQuery> query) {
  last_elapsed_query_ = query;
}

scoped_refptr<TimeElapsedTimerQuery> GPUTimingImpl::GetLastElapsedQuery() {
  return last_elapsed_query_;
}

void GPUTimingImpl::HandleBadQuery() {
  // Mark all queries as bad and signal an artificial disjoint value.
  next_good_timer_query_id_ = next_timer_query_id_;
  offset_valid_ = false;
  query_disjoint_count_ = ++disjoint_counter_;
}

bool GPUTimingImpl::IsGoodQueryID(uint32_t query_id) {
  return query_id >= next_good_timer_query_id_;
}

scoped_refptr<GPUTimingClient> GPUTimingImpl::CreateGPUTimingClient() {
  return new GPUTimingClient(this);
}

GPUTiming* GPUTiming::CreateGPUTiming(GLContextReal* context) {
  return new GPUTimingImpl(context);
}

GPUTiming::GPUTiming() {
}

GPUTiming::~GPUTiming() {
}

GPUTimer::~GPUTimer() {
}

void GPUTimer::Destroy(bool have_context) {
  if (have_context) {
    if (!end_requested_) {
      DCHECK(gpu_timing_client_->gpu_timing_);
      DCHECK(elapsed_timer_result_.get());
      gpu_timing_client_->gpu_timing_->EndElapsedTimeQuery(
          elapsed_timer_result_);
    }
  }
}

void GPUTimer::Start() {
  DCHECK(gpu_timing_client_->gpu_timing_);
  if (!use_elapsed_timer_)
    time_stamp_result_ = gpu_timing_client_->gpu_timing_->DoTimeStampQuery();

  elapsed_timer_result_ =
      gpu_timing_client_->gpu_timing_->BeginElapsedTimeQuery();
}

void GPUTimer::End() {
  DCHECK(elapsed_timer_result_.get());
  end_requested_ = true;
  gpu_timing_client_->gpu_timing_->EndElapsedTimeQuery(elapsed_timer_result_);
}

bool GPUTimer::IsAvailable() {
  if (!end_requested_)
    return false;
  if (!end_available_) {
    DCHECK(elapsed_timer_result_.get());
    if (elapsed_timer_result_->IsAvailable()) {
      end_available_ = true;
    } else {
      gpu_timing_client_->gpu_timing_->UpdateQueryResults();
      end_available_ = elapsed_timer_result_->IsAvailable();
    }
  }
  return end_available_;
}

void GPUTimer::GetStartEndTimestamps(int64* start, int64* end) {
  DCHECK(start && end);
  DCHECK(elapsed_timer_result_.get());
  DCHECK(IsAvailable());
  if (time_stamp_result_.get()) {
    DCHECK(time_stamp_result_->IsAvailable());
    const int64_t time_stamp = time_stamp_result_->GetStartValue();
    *start = time_stamp;
    *end = time_stamp + elapsed_timer_result_->GetDelta();
  } else {
    // Use estimation from elasped timer results.
    *start = elapsed_timer_result_->GetStartValue();
    *end = elapsed_timer_result_->GetEndValue();
  }
}

int64 GPUTimer::GetDeltaElapsed() {
  DCHECK(elapsed_timer_result_.get());
  DCHECK(IsAvailable());
  return elapsed_timer_result_->GetDelta();
}

GPUTimer::GPUTimer(scoped_refptr<GPUTimingClient> gpu_timing_client,
                   bool use_elapsed_timer)
    : use_elapsed_timer_(use_elapsed_timer),
      gpu_timing_client_(gpu_timing_client) {
}

GPUTimingClient::GPUTimingClient(GPUTimingImpl* gpu_timing)
    : gpu_timing_(gpu_timing) {
  if (gpu_timing) {
    timer_type_ = gpu_timing->GetTimerType();
    disjoint_counter_ = gpu_timing_->GetDisjointCount();
  }
}

scoped_ptr<GPUTimer> GPUTimingClient::CreateGPUTimer(bool prefer_elapsed_time) {
  prefer_elapsed_time |= (timer_type_ == GPUTiming::kTimerTypeEXT);
  if (gpu_timing_)
    prefer_elapsed_time |= gpu_timing_->IsForceTimeElapsedQuery();

  return make_scoped_ptr(new GPUTimer(this, prefer_elapsed_time));
}

bool GPUTimingClient::IsAvailable() {
  return timer_type_ != GPUTiming::kTimerTypeInvalid;
}

const char* GPUTimingClient::GetTimerTypeName() const {
  switch (timer_type_) {
    case GPUTiming::kTimerTypeDisjoint:
      return "GL_EXT_disjoint_timer_query";
    case GPUTiming::kTimerTypeARB:
      return "GL_ARB_timer_query";
    case GPUTiming::kTimerTypeEXT:
      return "GL_EXT_timer_query";
    default:
      return "Unknown";
  }
}

bool GPUTimingClient::CheckAndResetTimerErrors() {
  if (timer_type_ == GPUTiming::kTimerTypeDisjoint) {
    DCHECK(gpu_timing_ != nullptr);
    const uint32_t total_disjoint_count = gpu_timing_->GetDisjointCount();
    const bool disjoint_triggered = total_disjoint_count != disjoint_counter_;
    disjoint_counter_ = total_disjoint_count;
    return disjoint_triggered;
  }
  return false;
}

int64 GPUTimingClient::GetCurrentCPUTime() {
  DCHECK(gpu_timing_);
  return gpu_timing_->GetCurrentCPUTime();
}

void GPUTimingClient::SetCpuTimeForTesting(
    const base::Callback<int64(void)>& cpu_time) {
  DCHECK(gpu_timing_);
  gpu_timing_->SetCpuTimeForTesting(cpu_time);
}

bool GPUTimingClient::IsForceTimeElapsedQuery() {
  DCHECK(gpu_timing_);
  return gpu_timing_->IsForceTimeElapsedQuery();
}

void GPUTimingClient::ForceTimeElapsedQuery() {
  DCHECK(gpu_timing_);
  gpu_timing_->ForceTimeElapsedQuery();
}

GPUTimingClient::~GPUTimingClient() {
}

}  // namespace gfx
