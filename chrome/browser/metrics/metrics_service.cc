// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

//------------------------------------------------------------------------------
// Description of the life cycle of a instance of MetricsService.
//
//  OVERVIEW
//
// A MetricsService instance is typically created at application startup.  It is
// the central controller for the acquisition of log data, and the automatic
// transmission of that log data to an external server.  Its major job is to
// manage logs, grouping them for transmission, and transmitting them.  As part
// of its grouping, MS finalizes logs by including some just-in-time gathered
// memory statistics, snapshotting the current stats of numerous histograms,
// closing the logs, translating to protocol buffer format, and compressing the
// results for transmission.  Transmission includes submitting a compressed log
// as data in a URL-post, and retransmitting (or retaining at process
// termination) if the attempted transmission failed.  Retention across process
// terminations is done using the the PrefServices facilities. The retained logs
// (the ones that never got transmitted) are compressed and base64-encoded
// before being persisted.
//
// Logs fall into one of two categories: "initial logs," and "ongoing logs."
// There is at most one initial log sent for each complete run of Chrome (from
// startup, to browser shutdown).  An initial log is generally transmitted some
// short time (1 minute?) after startup, and includes stats such as recent crash
// info, the number and types of plugins, etc.  The external server's response
// to the initial log conceptually tells this MS if it should continue
// transmitting logs (during this session). The server response can actually be
// much more detailed, and always includes (at a minimum) how often additional
// ongoing logs should be sent.
//
// After the above initial log, a series of ongoing logs will be transmitted.
// The first ongoing log actually begins to accumulate information stating when
// the MS was first constructed.  Note that even though the initial log is
// commonly sent a full minute after startup, the initial log does not include
// much in the way of user stats.   The most common interlog period (delay)
// is 30 minutes. That time period starts when the first user action causes a
// logging event.  This means that if there is no user action, there may be long
// periods without any (ongoing) log transmissions.  Ongoing logs typically
// contain very detailed records of user activities (ex: opened tab, closed
// tab, fetched URL, maximized window, etc.)  In addition, just before an
// ongoing log is closed out, a call is made to gather memory statistics.  Those
// memory statistics are deposited into a histogram, and the log finalization
// code is then called.  In the finalization, a call to a Histogram server
// acquires a list of all local histograms that have been flagged for upload
// to the UMA server.  The finalization also acquires the most recent number
// of page loads, along with any counts of renderer or plugin crashes.
//
// When the browser shuts down, there will typically be a fragment of an ongoing
// log that has not yet been transmitted.  At shutdown time, that fragment is
// closed (including snapshotting histograms), and persisted, for potential
// transmission during a future run of the product.
//
// There are two slightly abnormal shutdown conditions.  There is a
// "disconnected scenario," and a "really fast startup and shutdown" scenario.
// In the "never connected" situation, the user has (during the running of the
// process) never established an internet connection.  As a result, attempts to
// transmit the initial log have failed, and a lot(?) of data has accumulated in
// the ongoing log (which didn't yet get closed, because there was never even a
// contemplation of sending it).  There is also a kindred "lost connection"
// situation, where a loss of connection prevented an ongoing log from being
// transmitted, and a (still open) log was stuck accumulating a lot(?) of data,
// while the earlier log retried its transmission.  In both of these
// disconnected situations, two logs need to be, and are, persistently stored
// for future transmission.
//
// The other unusual shutdown condition, termed "really fast startup and
// shutdown," involves the deliberate user termination of the process before
// the initial log is even formed or transmitted. In that situation, no logging
// is done, but the historical crash statistics remain (unlogged) for inclusion
// in a future run's initial log.  (i.e., we don't lose crash stats).
//
// With the above overview, we can now describe the state machine's various
// states, based on the State enum specified in the state_ member.  Those states
// are:
//
//  INITIALIZED,                   // Constructor was called.
//  INIT_TASK_SCHEDULED,           // Waiting for deferred init tasks to finish.
//  INIT_TASK_DONE,                // Waiting for timer to send initial log.
//  SENDING_INITIAL_STABILITY_LOG, // Initial stability log being sent.
//  SENDING_INITIAL_METRICS_LOG,   // Initial metrics log being sent.
//  SENDING_OLD_LOGS,              // Sending unsent logs from previous session.
//  SENDING_CURRENT_LOGS,          // Sending ongoing logs as they acrue.
//
// In more detail, we have:
//
//    INITIALIZED,            // Constructor was called.
// The MS has been constructed, but has taken no actions to compose the
// initial log.
//
//    INIT_TASK_SCHEDULED,    // Waiting for deferred init tasks to finish.
// Typically about 30 seconds after startup, a task is sent to a second thread
// (the file thread) to perform deferred (lower priority and slower)
// initialization steps such as getting the list of plugins.  That task will
// (when complete) make an async callback (via a Task) to indicate the
// completion.
//
//    INIT_TASK_DONE,         // Waiting for timer to send initial log.
// The callback has arrived, and it is now possible for an initial log to be
// created.  This callback typically arrives back less than one second after
// the deferred init task is dispatched.
//
//    SENDING_INITIAL_STABILITY_LOG,  // Initial stability log being sent.
// During initialization, if a crash occurred during the previous session, an
// initial stability log will be generated and registered with the log manager.
// This state will be entered if a stability log was prepared during metrics
// service initialization (in InitializeMetricsRecordingState()) and is waiting
// to be transmitted when it's time to send up the first log (per the reporting
// scheduler).  If there is no initial stability log (e.g. there was no previous
// crash), then this state will be skipped and the state will advance to
// SENDING_INITIAL_METRICS_LOG.
//
//    SENDING_INITIAL_METRICS_LOG,  // Initial metrics log being sent.
// This state is entered after the initial metrics log has been composed, and
// prepared for transmission.  This happens after SENDING_INITIAL_STABILITY_LOG
// if there was an initial stability log (see above).  It is also the case that
// any previously unsent logs have been loaded into instance variables for
// possible transmission.
//
//    SENDING_OLD_LOGS,       // Sending unsent logs from previous session.
// This state indicates that the initial log for this session has been
// successfully sent and it is now time to send any logs that were
// saved from previous sessions.  All such logs will be transmitted before
// exiting this state, and proceeding with ongoing logs from the current session
// (see next state).
//
//    SENDING_CURRENT_LOGS,   // Sending standard current logs as they accrue.
// Current logs are being accumulated.  Typically every 20 minutes a log is
// closed and finalized for transmission, at the same time as a new log is
// started.
//
// The progression through the above states is simple, and sequential, in the
// most common use cases.  States proceed from INITIAL to SENDING_CURRENT_LOGS,
// and remain in the latter until shutdown.
//
// The one unusual case is when the user asks that we stop logging.  When that
// happens, any staged (transmission in progress) log is persisted, and any log
// that is currently accumulating is also finalized and persisted.  We then
// regress back to the SEND_OLD_LOGS state in case the user enables log
// recording again during this session.  This way anything we have persisted
// will be sent automatically if/when we progress back to SENDING_CURRENT_LOG
// state.
//
// Another similar case is on mobile, when the application is backgrounded and
// then foregrounded again. Backgrounding created new "old" stored logs, so the
// state drops back from SENDING_CURRENT_LOGS to SENDING_OLD_LOGS so those logs
// will be sent.
//
// Also note that whenever we successfully send an old log, we mirror the list
// of logs into the PrefService. This ensures that IF we crash, we won't start
// up and retransmit our old logs again.
//
// Due to race conditions, it is always possible that a log file could be sent
// twice.  For example, if a log file is sent, but not yet acknowledged by
// the external server, and the user shuts down, then a copy of the log may be
// saved for re-transmission.  These duplicates could be filtered out server
// side, but are not expected to be a significant problem.
//
//
//------------------------------------------------------------------------------

#include "chrome/browser/metrics/metrics_service.h"

#include <algorithm>

#include "base/bind.h"
#include "base/callback.h"
#include "base/metrics/histogram.h"
#include "base/metrics/histogram_base.h"
#include "base/metrics/histogram_samples.h"
#include "base/metrics/sparse_histogram.h"
#include "base/metrics/statistics_recorder.h"
#include "base/prefs/pref_registry_simple.h"
#include "base/prefs/pref_service.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/platform_thread.h"
#include "base/threading/thread.h"
#include "base/threading/thread_restrictions.h"
#include "base/tracked_objects.h"
#include "base/values.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/io_thread.h"
#include "chrome/browser/metrics/chrome_stability_metrics_provider.h"
#include "chrome/browser/metrics/compression_utils.h"
#include "chrome/browser/metrics/gpu_metrics_provider.h"
#include "chrome/browser/metrics/metrics_log.h"
#include "chrome/browser/metrics/metrics_state_manager.h"
#include "chrome/browser/metrics/network_metrics_provider.h"
#include "chrome/browser/metrics/omnibox_metrics_provider.h"
#include "chrome/browser/metrics/tracking_synchronizer.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/variations/variations_util.h"
#include "components/metrics/metrics_log_base.h"
#include "components/metrics/metrics_log_manager.h"
#include "components/metrics/metrics_pref_names.h"
#include "components/metrics/metrics_reporting_scheduler.h"
#include "components/metrics/metrics_service_client.h"
#include "components/variations/entropy_provider.h"
#include "net/base/load_flags.h"
#include "net/url_request/url_fetcher.h"

#if defined(ENABLE_PLUGINS)
// TODO(asvitkine): Move this out of MetricsService.
#include "chrome/browser/metrics/plugin_metrics_provider.h"
#endif

#if defined(OS_WIN)
#include "chrome/browser/metrics/google_update_metrics_provider_win.h"
#endif

#if defined(OS_ANDROID)
// TODO(asvitkine): Move this out of MetricsService.
#include "chrome/browser/metrics/android_metrics_provider.h"
#endif

using base::Time;
using metrics::MetricsLogManager;

namespace {

// Check to see that we're being called on only one thread.
bool IsSingleThreaded() {
  static base::PlatformThreadId thread_id = 0;
  if (!thread_id)
    thread_id = base::PlatformThread::CurrentId();
  return base::PlatformThread::CurrentId() == thread_id;
}

// The delay, in seconds, after starting recording before doing expensive
// initialization work.
#if defined(OS_ANDROID) || defined(OS_IOS)
// On mobile devices, a significant portion of sessions last less than a minute.
// Use a shorter timer on these platforms to avoid losing data.
// TODO(dfalcantara): To avoid delaying startup, tighten up initialization so
//                    that it occurs after the user gets their initial page.
const int kInitializationDelaySeconds = 5;
#else
const int kInitializationDelaySeconds = 30;
#endif

// The maximum number of events in a log uploaded to the UMA server.
const int kEventLimit = 2400;

// If an upload fails, and the transmission was over this byte count, then we
// will discard the log, and not try to retransmit it.  We also don't persist
// the log to the prefs for transmission during the next chrome session if this
// limit is exceeded.
const size_t kUploadLogAvoidRetransmitSize = 50000;

// Interval, in minutes, between state saves.
const int kSaveStateIntervalMinutes = 5;

// The metrics server's URL.
const char kServerUrl[] = "https://clients4.google.com/uma/v2";

// The MIME type for the uploaded metrics data.
const char kMimeType[] = "application/vnd.chrome.uma";

enum ResponseStatus {
  UNKNOWN_FAILURE,
  SUCCESS,
  BAD_REQUEST,  // Invalid syntax or log too large.
  NO_RESPONSE,
  NUM_RESPONSE_STATUSES
};

ResponseStatus ResponseCodeToStatus(int response_code) {
  switch (response_code) {
    case 200:
      return SUCCESS;
    case 400:
      return BAD_REQUEST;
    case net::URLFetcher::RESPONSE_CODE_INVALID:
      return NO_RESPONSE;
    default:
      return UNKNOWN_FAILURE;
  }
}

void MarkAppCleanShutdownAndCommit(PrefService* local_state) {
  local_state->SetBoolean(prefs::kStabilityExitedCleanly, true);
  local_state->SetInteger(prefs::kStabilityExecutionPhase,
                          MetricsService::SHUTDOWN_COMPLETE);
  // Start writing right away (write happens on a different thread).
  local_state->CommitPendingWrite();
}

}  // namespace


SyntheticTrialGroup::SyntheticTrialGroup(uint32 trial, uint32 group) {
  id.name = trial;
  id.group = group;
}

SyntheticTrialGroup::~SyntheticTrialGroup() {
}

// static
MetricsService::ShutdownCleanliness MetricsService::clean_shutdown_status_ =
    MetricsService::CLEANLY_SHUTDOWN;

MetricsService::ExecutionPhase MetricsService::execution_phase_ =
    MetricsService::UNINITIALIZED_PHASE;

// static
void MetricsService::RegisterPrefs(PrefRegistrySimple* registry) {
  DCHECK(IsSingleThreaded());
  metrics::MetricsStateManager::RegisterPrefs(registry);

  registry->RegisterInt64Pref(prefs::kStabilityLaunchTimeSec, 0);
  registry->RegisterInt64Pref(prefs::kStabilityLastTimestampSec, 0);
  registry->RegisterStringPref(prefs::kStabilityStatsVersion, std::string());
  registry->RegisterInt64Pref(prefs::kStabilityStatsBuildTime, 0);
  registry->RegisterBooleanPref(prefs::kStabilityExitedCleanly, true);
  registry->RegisterIntegerPref(prefs::kStabilityExecutionPhase,
                                UNINITIALIZED_PHASE);
  registry->RegisterBooleanPref(prefs::kStabilitySessionEndCompleted, true);
  registry->RegisterIntegerPref(prefs::kMetricsSessionID, -1);
  registry->RegisterIntegerPref(prefs::kStabilityLaunchCount, 0);
  registry->RegisterIntegerPref(prefs::kStabilityCrashCount, 0);
  registry->RegisterIntegerPref(prefs::kStabilityIncompleteSessionEndCount, 0);
  registry->RegisterIntegerPref(prefs::kStabilityBreakpadRegistrationFail, 0);
  registry->RegisterIntegerPref(prefs::kStabilityBreakpadRegistrationSuccess,
                                0);
  registry->RegisterIntegerPref(prefs::kStabilityDebuggerPresent, 0);
  registry->RegisterIntegerPref(prefs::kStabilityDebuggerNotPresent, 0);

  registry->RegisterStringPref(prefs::kStabilitySavedSystemProfile,
                               std::string());
  registry->RegisterStringPref(prefs::kStabilitySavedSystemProfileHash,
                               std::string());

  registry->RegisterListPref(metrics::prefs::kMetricsInitialLogs);
  registry->RegisterListPref(metrics::prefs::kMetricsOngoingLogs);

  registry->RegisterInt64Pref(prefs::kInstallDate, 0);
  registry->RegisterInt64Pref(prefs::kUninstallLaunchCount, 0);
  registry->RegisterInt64Pref(prefs::kUninstallMetricsUptimeSec, 0);
  registry->RegisterInt64Pref(prefs::kUninstallLastLaunchTimeSec, 0);
  registry->RegisterInt64Pref(prefs::kUninstallLastObservedRunTimeSec, 0);

  // TODO(asvitkine): Move this out of here.
  ChromeStabilityMetricsProvider::RegisterPrefs(registry);

#if defined(OS_ANDROID)
  // TODO(asvitkine): Move this out of here.
  AndroidMetricsProvider::RegisterPrefs(registry);
#endif  // defined(OS_ANDROID)

#if defined(ENABLE_PLUGINS)
  // TODO(asvitkine): Move this out of here.
  PluginMetricsProvider::RegisterPrefs(registry);
#endif
}

MetricsService::MetricsService(metrics::MetricsStateManager* state_manager,
                               metrics::MetricsServiceClient* client,
                               PrefService* local_state)
    : log_manager_(local_state, kUploadLogAvoidRetransmitSize),
      histogram_snapshot_manager_(this),
      state_manager_(state_manager),
      client_(client),
      local_state_(local_state),
      recording_active_(false),
      reporting_active_(false),
      test_mode_active_(false),
      state_(INITIALIZED),
      has_initial_stability_log_(false),
      idle_since_last_transmission_(false),
      session_id_(-1),
      self_ptr_factory_(this),
      state_saver_factory_(this),
      waiting_for_asynchronous_reporting_step_(false) {
  DCHECK(IsSingleThreaded());
  DCHECK(state_manager_);
  DCHECK(client_);
  DCHECK(local_state_);

#if defined(OS_ANDROID)
  // TODO(asvitkine): Move this out of MetricsService.
  RegisterMetricsProvider(
      scoped_ptr<metrics::MetricsProvider>(new AndroidMetricsProvider(
          local_state_)));
#endif  // defined(OS_ANDROID)

  // TODO(asvitkine): Move these out of MetricsService.
  RegisterMetricsProvider(
      scoped_ptr<metrics::MetricsProvider>(new NetworkMetricsProvider));
  RegisterMetricsProvider(
      scoped_ptr<metrics::MetricsProvider>(new OmniboxMetricsProvider));
  RegisterMetricsProvider(
      scoped_ptr<metrics::MetricsProvider>(new ChromeStabilityMetricsProvider));
  RegisterMetricsProvider(
      scoped_ptr<metrics::MetricsProvider>(new GPUMetricsProvider()));

#if defined(OS_WIN)
  google_update_metrics_provider_ = new GoogleUpdateMetricsProviderWin;
  RegisterMetricsProvider(scoped_ptr<metrics::MetricsProvider>(
      google_update_metrics_provider_));
#endif

#if defined(ENABLE_PLUGINS)
  plugin_metrics_provider_ = new PluginMetricsProvider(local_state_);
  RegisterMetricsProvider(scoped_ptr<metrics::MetricsProvider>(
      plugin_metrics_provider_));
#endif

}

MetricsService::~MetricsService() {
  DisableRecording();
}

void MetricsService::InitializeMetricsRecordingState() {
  InitializeMetricsState();

  base::Closure callback = base::Bind(&MetricsService::StartScheduledUpload,
                                      self_ptr_factory_.GetWeakPtr());
  scheduler_.reset(new MetricsReportingScheduler(callback));
}

void MetricsService::Start() {
  HandleIdleSinceLastTransmission(false);
  EnableRecording();
  EnableReporting();
}

bool MetricsService::StartIfMetricsReportingEnabled() {
  const bool enabled = state_manager_->IsMetricsReportingEnabled();
  if (enabled)
    Start();
  return enabled;
}

void MetricsService::StartRecordingForTests() {
  test_mode_active_ = true;
  EnableRecording();
  DisableReporting();
}

void MetricsService::Stop() {
  HandleIdleSinceLastTransmission(false);
  DisableReporting();
  DisableRecording();
}

void MetricsService::EnableReporting() {
  if (reporting_active_)
    return;
  reporting_active_ = true;
  StartSchedulerIfNecessary();
}

void MetricsService::DisableReporting() {
  reporting_active_ = false;
}

std::string MetricsService::GetClientId() {
  return state_manager_->client_id();
}

scoped_ptr<const base::FieldTrial::EntropyProvider>
MetricsService::CreateEntropyProvider() {
  // TODO(asvitkine): Refactor the code so that MetricsService does not expose
  // this method.
  return state_manager_->CreateEntropyProvider();
}

void MetricsService::EnableRecording() {
  DCHECK(IsSingleThreaded());

  if (recording_active_)
    return;
  recording_active_ = true;

  state_manager_->ForceClientIdCreation();
  client_->SetClientID(state_manager_->client_id());
  if (!log_manager_.current_log())
    OpenNewLog();

  for (size_t i = 0; i < metrics_providers_.size(); ++i)
    metrics_providers_[i]->OnRecordingEnabled();

  base::RemoveActionCallback(action_callback_);
  action_callback_ = base::Bind(&MetricsService::OnUserAction,
                                base::Unretained(this));
  base::AddActionCallback(action_callback_);
}

void MetricsService::DisableRecording() {
  DCHECK(IsSingleThreaded());

  if (!recording_active_)
    return;
  recording_active_ = false;

  base::RemoveActionCallback(action_callback_);

  for (size_t i = 0; i < metrics_providers_.size(); ++i)
    metrics_providers_[i]->OnRecordingDisabled();

  PushPendingLogsToPersistentStorage();
  DCHECK(!log_manager_.has_staged_log());
}

bool MetricsService::recording_active() const {
  DCHECK(IsSingleThreaded());
  return recording_active_;
}

bool MetricsService::reporting_active() const {
  DCHECK(IsSingleThreaded());
  return reporting_active_;
}

void MetricsService::RecordDelta(const base::HistogramBase& histogram,
                                 const base::HistogramSamples& snapshot) {
  log_manager_.current_log()->RecordHistogramDelta(histogram.histogram_name(),
                                                   snapshot);
}

void MetricsService::InconsistencyDetected(
    base::HistogramBase::Inconsistency problem) {
  UMA_HISTOGRAM_ENUMERATION("Histogram.InconsistenciesBrowser",
                            problem, base::HistogramBase::NEVER_EXCEEDED_VALUE);
}

void MetricsService::UniqueInconsistencyDetected(
    base::HistogramBase::Inconsistency problem) {
  UMA_HISTOGRAM_ENUMERATION("Histogram.InconsistenciesBrowserUnique",
                            problem, base::HistogramBase::NEVER_EXCEEDED_VALUE);
}

void MetricsService::InconsistencyDetectedInLoggedCount(int amount) {
  UMA_HISTOGRAM_COUNTS("Histogram.InconsistentSnapshotBrowser",
                       std::abs(amount));
}

void MetricsService::HandleIdleSinceLastTransmission(bool in_idle) {
  // If there wasn't a lot of action, maybe the computer was asleep, in which
  // case, the log transmissions should have stopped.  Here we start them up
  // again.
  if (!in_idle && idle_since_last_transmission_)
    StartSchedulerIfNecessary();
  idle_since_last_transmission_ = in_idle;
}

void MetricsService::OnApplicationNotIdle() {
  if (recording_active_)
    HandleIdleSinceLastTransmission(false);
}

void MetricsService::RecordStartOfSessionEnd() {
  LogCleanShutdown();
  RecordBooleanPrefValue(prefs::kStabilitySessionEndCompleted, false);
}

void MetricsService::RecordCompletedSessionEnd() {
  LogCleanShutdown();
  RecordBooleanPrefValue(prefs::kStabilitySessionEndCompleted, true);
}

#if defined(OS_ANDROID) || defined(OS_IOS)
void MetricsService::OnAppEnterBackground() {
  scheduler_->Stop();

  MarkAppCleanShutdownAndCommit(local_state_);

  // At this point, there's no way of knowing when the process will be
  // killed, so this has to be treated similar to a shutdown, closing and
  // persisting all logs. Unlinke a shutdown, the state is primed to be ready
  // to continue logging and uploading if the process does return.
  if (recording_active() && state_ >= SENDING_INITIAL_STABILITY_LOG) {
    PushPendingLogsToPersistentStorage();
    // Persisting logs closes the current log, so start recording a new log
    // immediately to capture any background work that might be done before the
    // process is killed.
    OpenNewLog();
  }
}

void MetricsService::OnAppEnterForeground() {
  local_state_->SetBoolean(prefs::kStabilityExitedCleanly, false);
  StartSchedulerIfNecessary();
}
#else
void MetricsService::LogNeedForCleanShutdown(PrefService* local_state) {
  local_state->SetBoolean(prefs::kStabilityExitedCleanly, false);
  // Redundant setting to be sure we call for a clean shutdown.
  clean_shutdown_status_ = NEED_TO_SHUTDOWN;
}
#endif  // defined(OS_ANDROID) || defined(OS_IOS)

// static
void MetricsService::SetExecutionPhase(ExecutionPhase execution_phase,
                                       PrefService* local_state) {
  execution_phase_ = execution_phase;
  local_state->SetInteger(prefs::kStabilityExecutionPhase, execution_phase_);
}

void MetricsService::RecordBreakpadRegistration(bool success) {
  if (!success)
    IncrementPrefValue(prefs::kStabilityBreakpadRegistrationFail);
  else
    IncrementPrefValue(prefs::kStabilityBreakpadRegistrationSuccess);
}

void MetricsService::RecordBreakpadHasDebugger(bool has_debugger) {
  if (!has_debugger)
    IncrementPrefValue(prefs::kStabilityDebuggerNotPresent);
  else
    IncrementPrefValue(prefs::kStabilityDebuggerPresent);
}

//------------------------------------------------------------------------------
// private methods
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Initialization methods

void MetricsService::InitializeMetricsState() {
  local_state_->SetString(prefs::kStabilityStatsVersion,
                          client_->GetVersionString());
  local_state_->SetInt64(prefs::kStabilityStatsBuildTime,
                         MetricsLog::GetBuildTime());

  session_id_ = local_state_->GetInteger(prefs::kMetricsSessionID);

  if (!local_state_->GetBoolean(prefs::kStabilityExitedCleanly)) {
    IncrementPrefValue(prefs::kStabilityCrashCount);
    // Reset flag, and wait until we call LogNeedForCleanShutdown() before
    // monitoring.
    local_state_->SetBoolean(prefs::kStabilityExitedCleanly, true);

    // TODO(rtenneti): On windows, consider saving/getting execution_phase from
    // the registry.
    int execution_phase =
        local_state_->GetInteger(prefs::kStabilityExecutionPhase);
    UMA_HISTOGRAM_SPARSE_SLOWLY("Chrome.Browser.CrashedExecutionPhase",
                                execution_phase);

    // If the previous session didn't exit cleanly, then prepare an initial
    // stability log if UMA is enabled.
    if (state_manager_->IsMetricsReportingEnabled())
      PrepareInitialStabilityLog();
  }

  // Update session ID.
  ++session_id_;
  local_state_->SetInteger(prefs::kMetricsSessionID, session_id_);

  // Stability bookkeeping
  IncrementPrefValue(prefs::kStabilityLaunchCount);

  DCHECK_EQ(UNINITIALIZED_PHASE, execution_phase_);
  SetExecutionPhase(START_METRICS_RECORDING, local_state_);

  if (!local_state_->GetBoolean(prefs::kStabilitySessionEndCompleted)) {
    IncrementPrefValue(prefs::kStabilityIncompleteSessionEndCount);
    // This is marked false when we get a WM_ENDSESSION.
    local_state_->SetBoolean(prefs::kStabilitySessionEndCompleted, true);
  }

  // Call GetUptimes() for the first time, thus allowing all later calls
  // to record incremental uptimes accurately.
  base::TimeDelta ignored_uptime_parameter;
  base::TimeDelta startup_uptime;
  GetUptimes(local_state_, &startup_uptime, &ignored_uptime_parameter);
  DCHECK_EQ(0, startup_uptime.InMicroseconds());
  // For backwards compatibility, leave this intact in case Omaha is checking
  // them.  prefs::kStabilityLastTimestampSec may also be useless now.
  // TODO(jar): Delete these if they have no uses.
  local_state_->SetInt64(prefs::kStabilityLaunchTimeSec, Time::Now().ToTimeT());

  // Bookkeeping for the uninstall metrics.
  IncrementLongPrefsValue(prefs::kUninstallLaunchCount);

  // Kick off the process of saving the state (so the uptime numbers keep
  // getting updated) every n minutes.
  ScheduleNextStateSave();
}

void MetricsService::OnInitTaskGotHardwareClass() {
  DCHECK_EQ(INIT_TASK_SCHEDULED, state_);

  const base::Closure got_plugin_info_callback =
      base::Bind(&MetricsService::OnInitTaskGotPluginInfo,
                 self_ptr_factory_.GetWeakPtr());

#if defined(ENABLE_PLUGINS)
  plugin_metrics_provider_->GetPluginInformation(got_plugin_info_callback);
#else
  got_plugin_info_callback.Run();
#endif
}

void MetricsService::OnInitTaskGotPluginInfo() {
  DCHECK_EQ(INIT_TASK_SCHEDULED, state_);

  const base::Closure got_metrics_callback =
      base::Bind(&MetricsService::OnInitTaskGotGoogleUpdateData,
                 self_ptr_factory_.GetWeakPtr());

#if defined(OS_WIN) && defined(GOOGLE_CHROME_BUILD)
  google_update_metrics_provider_->GetGoogleUpdateData(got_metrics_callback);
#else
  got_metrics_callback.Run();
#endif
}

void MetricsService::OnInitTaskGotGoogleUpdateData() {
  DCHECK_EQ(INIT_TASK_SCHEDULED, state_);

  // Start the next part of the init task: fetching performance data.  This will
  // call into |FinishedReceivingProfilerData()| when the task completes.
  chrome_browser_metrics::TrackingSynchronizer::FetchProfilerDataAsynchronously(
      self_ptr_factory_.GetWeakPtr());
}

void MetricsService::OnUserAction(const std::string& action) {
  if (!ShouldLogEvents())
    return;

  log_manager_.current_log()->RecordUserAction(action);
  HandleIdleSinceLastTransmission(false);
}

void MetricsService::ReceivedProfilerData(
    const tracked_objects::ProcessDataSnapshot& process_data,
    int process_type) {
  DCHECK_EQ(INIT_TASK_SCHEDULED, state_);

  // Upon the first callback, create the initial log so that we can immediately
  // save the profiler data.
  if (!initial_metrics_log_.get()) {
    initial_metrics_log_ = CreateLog(MetricsLog::ONGOING_LOG);
    NotifyOnDidCreateMetricsLog();
  }

  initial_metrics_log_->RecordProfilerData(process_data, process_type);
}

void MetricsService::FinishedReceivingProfilerData() {
  DCHECK_EQ(INIT_TASK_SCHEDULED, state_);
  state_ = INIT_TASK_DONE;
  scheduler_->InitTaskComplete();
}

void MetricsService::GetUptimes(PrefService* pref,
                                base::TimeDelta* incremental_uptime,
                                base::TimeDelta* uptime) {
  base::TimeTicks now = base::TimeTicks::Now();
  // If this is the first call, init |first_updated_time_| and
  // |last_updated_time_|.
  if (last_updated_time_.is_null()) {
    first_updated_time_ = now;
    last_updated_time_ = now;
  }
  *incremental_uptime = now - last_updated_time_;
  *uptime = now - first_updated_time_;
  last_updated_time_ = now;

  const int64 incremental_time_secs = incremental_uptime->InSeconds();
  if (incremental_time_secs > 0) {
    int64 metrics_uptime = pref->GetInt64(prefs::kUninstallMetricsUptimeSec);
    metrics_uptime += incremental_time_secs;
    pref->SetInt64(prefs::kUninstallMetricsUptimeSec, metrics_uptime);
  }
}

void MetricsService::AddObserver(MetricsServiceObserver* observer) {
  DCHECK(thread_checker_.CalledOnValidThread());
  observers_.AddObserver(observer);
}

void MetricsService::RemoveObserver(MetricsServiceObserver* observer) {
  DCHECK(thread_checker_.CalledOnValidThread());
  observers_.RemoveObserver(observer);
}

void MetricsService::NotifyOnDidCreateMetricsLog() {
  DCHECK(thread_checker_.CalledOnValidThread());
  FOR_EACH_OBSERVER(
      MetricsServiceObserver, observers_, OnDidCreateMetricsLog());
  for (size_t i = 0; i < metrics_providers_.size(); ++i)
    metrics_providers_[i]->OnDidCreateMetricsLog();
}

//------------------------------------------------------------------------------
// State save methods

void MetricsService::ScheduleNextStateSave() {
  state_saver_factory_.InvalidateWeakPtrs();

  base::MessageLoop::current()->PostDelayedTask(FROM_HERE,
      base::Bind(&MetricsService::SaveLocalState,
                 state_saver_factory_.GetWeakPtr()),
      base::TimeDelta::FromMinutes(kSaveStateIntervalMinutes));
}

void MetricsService::SaveLocalState() {
  RecordCurrentState(local_state_);

  // TODO(jar):110021 Does this run down the batteries????
  ScheduleNextStateSave();
}


//------------------------------------------------------------------------------
// Recording control methods

void MetricsService::OpenNewLog() {
  DCHECK(!log_manager_.current_log());

  log_manager_.BeginLoggingWithLog(
      CreateLog(MetricsLog::ONGOING_LOG).PassAs<metrics::MetricsLogBase>());
  NotifyOnDidCreateMetricsLog();
  if (state_ == INITIALIZED) {
    // We only need to schedule that run once.
    state_ = INIT_TASK_SCHEDULED;

    content::BrowserThread::PostDelayedTask(
        content::BrowserThread::UI,
        FROM_HERE,
        base::Bind(&MetricsService::StartGatheringMetrics,
                   self_ptr_factory_.GetWeakPtr()),
        base::TimeDelta::FromSeconds(kInitializationDelaySeconds));
  }
}

void MetricsService::StartGatheringMetrics() {
  // TODO(blundell): Move all initial metrics gathering to
  // ChromeMetricsServiceClient.
  client_->StartGatheringMetrics(
      base::Bind(&MetricsService::OnInitTaskGotHardwareClass,
                 self_ptr_factory_.GetWeakPtr()));
}

void MetricsService::CloseCurrentLog() {
  if (!log_manager_.current_log())
    return;

  // TODO(jar): Integrate bounds on log recording more consistently, so that we
  // can stop recording logs that are too big much sooner.
  if (log_manager_.current_log()->num_events() > kEventLimit) {
    UMA_HISTOGRAM_COUNTS("UMA.Discarded Log Events",
                         log_manager_.current_log()->num_events());
    log_manager_.DiscardCurrentLog();
    OpenNewLog();  // Start trivial log to hold our histograms.
  }

  // Put incremental data (histogram deltas, and realtime stats deltas) at the
  // end of all log transmissions (initial log handles this separately).
  // RecordIncrementalStabilityElements only exists on the derived
  // MetricsLog class.
  MetricsLog* current_log =
      static_cast<MetricsLog*>(log_manager_.current_log());
  DCHECK(current_log);
  std::vector<variations::ActiveGroupId> synthetic_trials;
  GetCurrentSyntheticFieldTrials(&synthetic_trials);
  current_log->RecordEnvironment(metrics_providers_.get(), synthetic_trials);
  base::TimeDelta incremental_uptime;
  base::TimeDelta uptime;
  GetUptimes(local_state_, &incremental_uptime, &uptime);
  current_log->RecordStabilityMetrics(metrics_providers_.get(),
                                      incremental_uptime, uptime);

  RecordCurrentHistograms();
  current_log->RecordGeneralMetrics(metrics_providers_.get());

  log_manager_.FinishCurrentLog();
}

void MetricsService::PushPendingLogsToPersistentStorage() {
  if (state_ < SENDING_INITIAL_STABILITY_LOG)
    return;  // We didn't and still don't have time to get plugin list etc.

  if (log_manager_.has_staged_log()) {
    // We may race here, and send second copy of the log later.
    metrics::PersistedLogs::StoreType store_type;
    if (current_fetch_.get())
      store_type = metrics::PersistedLogs::PROVISIONAL_STORE;
    else
      store_type = metrics::PersistedLogs::NORMAL_STORE;
    log_manager_.StoreStagedLogAsUnsent(store_type);
  }
  DCHECK(!log_manager_.has_staged_log());
  CloseCurrentLog();
  log_manager_.PersistUnsentLogs();

  // If there was a staged and/or current log, then there is now at least one
  // log waiting to be uploaded.
  if (log_manager_.has_unsent_logs())
    state_ = SENDING_OLD_LOGS;
}

//------------------------------------------------------------------------------
// Transmission of logs methods

void MetricsService::StartSchedulerIfNecessary() {
  // Never schedule cutting or uploading of logs in test mode.
  if (test_mode_active_)
    return;

  // Even if reporting is disabled, the scheduler is needed to trigger the
  // creation of the initial log, which must be done in order for any logs to be
  // persisted on shutdown or backgrounding.
  if (recording_active() &&
      (reporting_active() || state_ < SENDING_INITIAL_STABILITY_LOG)) {
    scheduler_->Start();
  }
}

void MetricsService::StartScheduledUpload() {
  // If we're getting no notifications, then the log won't have much in it, and
  // it's possible the computer is about to go to sleep, so don't upload and
  // stop the scheduler.
  // If recording has been turned off, the scheduler doesn't need to run.
  // If reporting is off, proceed if the initial log hasn't been created, since
  // that has to happen in order for logs to be cut and stored when persisting.
  // TODO(stuartmorgan): Call Stop() on the scheduler when reporting and/or
  // recording are turned off instead of letting it fire and then aborting.
  if (idle_since_last_transmission_ ||
      !recording_active() ||
      (!reporting_active() && state_ >= SENDING_INITIAL_STABILITY_LOG)) {
    scheduler_->Stop();
    scheduler_->UploadCancelled();
    return;
  }

  // If the callback was to upload an old log, but there no longer is one,
  // just report success back to the scheduler to begin the ongoing log
  // callbacks.
  // TODO(stuartmorgan): Consider removing the distinction between
  // SENDING_OLD_LOGS and SENDING_CURRENT_LOGS to simplify the state machine
  // now that the log upload flow is the same for both modes.
  if (state_ == SENDING_OLD_LOGS && !log_manager_.has_unsent_logs()) {
    state_ = SENDING_CURRENT_LOGS;
    scheduler_->UploadFinished(true /* healthy */, false /* no unsent logs */);
    return;
  }
  // If there are unsent logs, send the next one. If not, start the asynchronous
  // process of finalizing the current log for upload.
  if (state_ == SENDING_OLD_LOGS) {
    DCHECK(log_manager_.has_unsent_logs());
    log_manager_.StageNextLogForUpload();
    SendStagedLog();
  } else {
    client_->CollectFinalMetrics(
        base::Bind(&MetricsService::OnFinalLogInfoCollectionDone,
                   self_ptr_factory_.GetWeakPtr()));
  }
}

void MetricsService::OnFinalLogInfoCollectionDone() {
  // If somehow there is a fetch in progress, we return and hope things work
  // out. The scheduler isn't informed since if this happens, the scheduler
  // will get a response from the upload.
  DCHECK(!current_fetch_.get());
  if (current_fetch_.get())
    return;

  // Abort if metrics were turned off during the final info gathering.
  if (!recording_active()) {
    scheduler_->Stop();
    scheduler_->UploadCancelled();
    return;
  }

  StageNewLog();

  // If logs shouldn't be uploaded, stop here. It's important that this check
  // be after StageNewLog(), otherwise the previous logs will never be loaded,
  // and thus the open log won't be persisted.
  // TODO(stuartmorgan): This is unnecessarily complicated; restructure loading
  // of previous logs to not require running part of the upload logic.
  // http://crbug.com/157337
  if (!reporting_active()) {
    scheduler_->Stop();
    scheduler_->UploadCancelled();
    return;
  }

  SendStagedLog();
}

void MetricsService::StageNewLog() {
  if (log_manager_.has_staged_log())
    return;

  switch (state_) {
    case INITIALIZED:
    case INIT_TASK_SCHEDULED:  // We should be further along by now.
      NOTREACHED();
      return;

    case INIT_TASK_DONE:
      if (has_initial_stability_log_) {
        // There's an initial stability log, ready to send.
        log_manager_.StageNextLogForUpload();
        has_initial_stability_log_ = false;
        // Note: No need to call LoadPersistedUnsentLogs() here because unsent
        // logs have already been loaded by PrepareInitialStabilityLog().
        state_ = SENDING_INITIAL_STABILITY_LOG;
      } else {
        PrepareInitialMetricsLog();
        // Load unsent logs (if any) from local state.
        log_manager_.LoadPersistedUnsentLogs();
        state_ = SENDING_INITIAL_METRICS_LOG;
      }
      break;

    case SENDING_OLD_LOGS:
      NOTREACHED();  // Shouldn't be staging a new log during old log sending.
      return;

    case SENDING_CURRENT_LOGS:
      CloseCurrentLog();
      OpenNewLog();
      log_manager_.StageNextLogForUpload();
      break;

    default:
      NOTREACHED();
      return;
  }

  DCHECK(log_manager_.has_staged_log());
}

void MetricsService::PrepareInitialStabilityLog() {
  DCHECK_EQ(INITIALIZED, state_);
  DCHECK_NE(0, local_state_->GetInteger(prefs::kStabilityCrashCount));

  scoped_ptr<MetricsLog> initial_stability_log(
      CreateLog(MetricsLog::INITIAL_STABILITY_LOG));

  // Do not call NotifyOnDidCreateMetricsLog here because the stability
  // log describes stats from the _previous_ session.

  if (!initial_stability_log->LoadSavedEnvironmentFromPrefs())
    return;

  log_manager_.LoadPersistedUnsentLogs();

  log_manager_.PauseCurrentLog();
  log_manager_.BeginLoggingWithLog(
      initial_stability_log.PassAs<metrics::MetricsLogBase>());

  // Note: Some stability providers may record stability stats via histograms,
  //       so this call has to be after BeginLoggingWithLog().
  MetricsLog* current_log =
      static_cast<MetricsLog*>(log_manager_.current_log());
  current_log->RecordStabilityMetrics(metrics_providers_.get(),
                                      base::TimeDelta(), base::TimeDelta());
  RecordCurrentStabilityHistograms();

  // Note: RecordGeneralMetrics() intentionally not called since this log is for
  //       stability stats from a previous session only.

  log_manager_.FinishCurrentLog();
  log_manager_.ResumePausedLog();

  // Store unsent logs, including the stability log that was just saved, so
  // that they're not lost in case of a crash before upload time.
  log_manager_.PersistUnsentLogs();

  has_initial_stability_log_ = true;
}

void MetricsService::PrepareInitialMetricsLog() {
  DCHECK(state_ == INIT_TASK_DONE || state_ == SENDING_INITIAL_STABILITY_LOG);

  std::vector<variations::ActiveGroupId> synthetic_trials;
  GetCurrentSyntheticFieldTrials(&synthetic_trials);
  initial_metrics_log_->RecordEnvironment(metrics_providers_.get(),
                                          synthetic_trials);
  base::TimeDelta incremental_uptime;
  base::TimeDelta uptime;
  GetUptimes(local_state_, &incremental_uptime, &uptime);

  // Histograms only get written to the current log, so make the new log current
  // before writing them.
  log_manager_.PauseCurrentLog();
  log_manager_.BeginLoggingWithLog(
      initial_metrics_log_.PassAs<metrics::MetricsLogBase>());

  // Note: Some stability providers may record stability stats via histograms,
  //       so this call has to be after BeginLoggingWithLog().
  MetricsLog* current_log =
      static_cast<MetricsLog*>(log_manager_.current_log());
  current_log->RecordStabilityMetrics(metrics_providers_.get(),
                                      base::TimeDelta(), base::TimeDelta());
  RecordCurrentHistograms();

  current_log->RecordGeneralMetrics(metrics_providers_.get());

  log_manager_.FinishCurrentLog();
  log_manager_.ResumePausedLog();

  DCHECK(!log_manager_.has_staged_log());
  log_manager_.StageNextLogForUpload();
}

void MetricsService::SendStagedLog() {
  DCHECK(log_manager_.has_staged_log());

  PrepareFetchWithStagedLog();

  bool upload_created = (current_fetch_.get() != NULL);
  UMA_HISTOGRAM_BOOLEAN("UMA.UploadCreation", upload_created);
  if (!upload_created) {
    // Compression failed, and log discarded :-/.
    // Skip this upload and hope things work out next time.
    log_manager_.DiscardStagedLog();
    scheduler_->UploadCancelled();
    return;
  }

  DCHECK(!waiting_for_asynchronous_reporting_step_);
  waiting_for_asynchronous_reporting_step_ = true;

  current_fetch_->Start();

  HandleIdleSinceLastTransmission(true);
}

void MetricsService::PrepareFetchWithStagedLog() {
  DCHECK(log_manager_.has_staged_log());

  // Prepare the protobuf version.
  DCHECK(!current_fetch_.get());
  if (log_manager_.has_staged_log()) {
    current_fetch_.reset(net::URLFetcher::Create(
        GURL(kServerUrl), net::URLFetcher::POST, this));
    current_fetch_->SetRequestContext(
        g_browser_process->system_request_context());

    std::string log_text = log_manager_.staged_log();
    std::string compressed_log_text;
    bool compression_successful = chrome::GzipCompress(log_text,
                                                       &compressed_log_text);
    DCHECK(compression_successful);
    if (compression_successful) {
      current_fetch_->SetUploadData(kMimeType, compressed_log_text);
      // Tell the server that we're uploading gzipped protobufs.
      current_fetch_->SetExtraRequestHeaders("content-encoding: gzip");
      const std::string hash =
          base::HexEncode(log_manager_.staged_log_hash().data(),
                          log_manager_.staged_log_hash().size());
      DCHECK(!hash.empty());
      current_fetch_->AddExtraRequestHeader("X-Chrome-UMA-Log-SHA1: " + hash);
      UMA_HISTOGRAM_PERCENTAGE(
          "UMA.ProtoCompressionRatio",
          100 * compressed_log_text.size() / log_text.size());
      UMA_HISTOGRAM_CUSTOM_COUNTS(
          "UMA.ProtoGzippedKBSaved",
          (log_text.size() - compressed_log_text.size()) / 1024,
          1, 2000, 50);
    }

    // We already drop cookies server-side, but we might as well strip them out
    // client-side as well.
    current_fetch_->SetLoadFlags(net::LOAD_DO_NOT_SAVE_COOKIES |
                                 net::LOAD_DO_NOT_SEND_COOKIES);
  }
}

void MetricsService::OnURLFetchComplete(const net::URLFetcher* source) {
  DCHECK(waiting_for_asynchronous_reporting_step_);

  // We're not allowed to re-use the existing |URLFetcher|s, so free them here.
  // Note however that |source| is aliased to the fetcher, so we should be
  // careful not to delete it too early.
  DCHECK_EQ(current_fetch_.get(), source);
  scoped_ptr<net::URLFetcher> s(current_fetch_.Pass());

  int response_code = source->GetResponseCode();

  // Log a histogram to track response success vs. failure rates.
  UMA_HISTOGRAM_ENUMERATION("UMA.UploadResponseStatus.Protobuf",
                            ResponseCodeToStatus(response_code),
                            NUM_RESPONSE_STATUSES);

  // If the upload was provisionally stored, drop it now that the upload is
  // known to have gone through.
  log_manager_.DiscardLastProvisionalStore();

  bool upload_succeeded = response_code == 200;

  // Provide boolean for error recovery (allow us to ignore response_code).
  bool discard_log = false;
  const size_t log_size = log_manager_.staged_log().length();
  if (!upload_succeeded && log_size > kUploadLogAvoidRetransmitSize) {
    UMA_HISTOGRAM_COUNTS("UMA.Large Rejected Log was Discarded",
                         static_cast<int>(log_size));
    discard_log = true;
  } else if (response_code == 400) {
    // Bad syntax.  Retransmission won't work.
    discard_log = true;
  }

  if (upload_succeeded || discard_log)
    log_manager_.DiscardStagedLog();

  waiting_for_asynchronous_reporting_step_ = false;

  if (!log_manager_.has_staged_log()) {
    switch (state_) {
      case SENDING_INITIAL_STABILITY_LOG:
        // Store the updated list to disk now that the removed log is uploaded.
        log_manager_.PersistUnsentLogs();
        PrepareInitialMetricsLog();
        SendStagedLog();
        state_ = SENDING_INITIAL_METRICS_LOG;
        break;

      case SENDING_INITIAL_METRICS_LOG:
        // The initial metrics log never gets persisted to local state, so it's
        // not necessary to call log_manager_.PersistUnsentLogs() here.
        // TODO(asvitkine): It should be persisted like the initial stability
        // log and old unsent logs. http://crbug.com/328417
        state_ = log_manager_.has_unsent_logs() ? SENDING_OLD_LOGS
                                                : SENDING_CURRENT_LOGS;
        break;

      case SENDING_OLD_LOGS:
        // Store the updated list to disk now that the removed log is uploaded.
        log_manager_.PersistUnsentLogs();
        if (!log_manager_.has_unsent_logs())
          state_ = SENDING_CURRENT_LOGS;
        break;

      case SENDING_CURRENT_LOGS:
        break;

      default:
        NOTREACHED();
        break;
    }

    if (log_manager_.has_unsent_logs())
      DCHECK_LT(state_, SENDING_CURRENT_LOGS);
  }

  // Error 400 indicates a problem with the log, not with the server, so
  // don't consider that a sign that the server is in trouble.
  bool server_is_healthy = upload_succeeded || response_code == 400;
  // Don't notify the scheduler that the upload is finished if we've only sent
  // the initial stability log, but not yet the initial metrics log (treat the
  // two as a single unit of work as far as the scheduler is concerned).
  if (state_ != SENDING_INITIAL_METRICS_LOG) {
    scheduler_->UploadFinished(server_is_healthy,
                               log_manager_.has_unsent_logs());
  }

  if (server_is_healthy)
    client_->OnLogUploadComplete();
}

void MetricsService::IncrementPrefValue(const char* path) {
  int value = local_state_->GetInteger(path);
  local_state_->SetInteger(path, value + 1);
}

void MetricsService::IncrementLongPrefsValue(const char* path) {
  int64 value = local_state_->GetInt64(path);
  local_state_->SetInt64(path, value + 1);
}

bool MetricsService::UmaMetricsProperlyShutdown() {
  CHECK(clean_shutdown_status_ == CLEANLY_SHUTDOWN ||
        clean_shutdown_status_ == NEED_TO_SHUTDOWN);
  return clean_shutdown_status_ == CLEANLY_SHUTDOWN;
}

void MetricsService::RegisterSyntheticFieldTrial(
    const SyntheticTrialGroup& trial) {
  for (size_t i = 0; i < synthetic_trial_groups_.size(); ++i) {
    if (synthetic_trial_groups_[i].id.name == trial.id.name) {
      if (synthetic_trial_groups_[i].id.group != trial.id.group) {
        synthetic_trial_groups_[i].id.group = trial.id.group;
        synthetic_trial_groups_[i].start_time = base::TimeTicks::Now();
      }
      return;
    }
  }

  SyntheticTrialGroup trial_group = trial;
  trial_group.start_time = base::TimeTicks::Now();
  synthetic_trial_groups_.push_back(trial_group);
}

void MetricsService::RegisterMetricsProvider(
    scoped_ptr<metrics::MetricsProvider> provider) {
  DCHECK_EQ(INITIALIZED, state_);
  metrics_providers_.push_back(provider.release());
}

void MetricsService::CheckForClonedInstall(
    scoped_refptr<base::SingleThreadTaskRunner> task_runner) {
  state_manager_->CheckForClonedInstall(task_runner);
}

void MetricsService::GetCurrentSyntheticFieldTrials(
    std::vector<variations::ActiveGroupId>* synthetic_trials) {
  DCHECK(synthetic_trials);
  synthetic_trials->clear();
  const MetricsLog* current_log =
      static_cast<const MetricsLog*>(log_manager_.current_log());
  for (size_t i = 0; i < synthetic_trial_groups_.size(); ++i) {
    if (synthetic_trial_groups_[i].start_time <= current_log->creation_time())
      synthetic_trials->push_back(synthetic_trial_groups_[i].id);
  }
}

scoped_ptr<MetricsLog> MetricsService::CreateLog(MetricsLog::LogType log_type) {
  return make_scoped_ptr(new MetricsLog(state_manager_->client_id(),
                                        session_id_,
                                        log_type,
                                        client_,
                                        local_state_));
}

void MetricsService::RecordCurrentHistograms() {
  DCHECK(log_manager_.current_log());
  histogram_snapshot_manager_.PrepareDeltas(
      base::Histogram::kNoFlags, base::Histogram::kUmaTargetedHistogramFlag);
}

void MetricsService::RecordCurrentStabilityHistograms() {
  DCHECK(log_manager_.current_log());
  histogram_snapshot_manager_.PrepareDeltas(
      base::Histogram::kNoFlags, base::Histogram::kUmaStabilityHistogramFlag);
}

void MetricsService::LogCleanShutdown() {
  // Redundant hack to write pref ASAP.
  MarkAppCleanShutdownAndCommit(local_state_);

  // Redundant setting to assure that we always reset this value at shutdown
  // (and that we don't use some alternate path, and not call LogCleanShutdown).
  clean_shutdown_status_ = CLEANLY_SHUTDOWN;

  RecordBooleanPrefValue(prefs::kStabilityExitedCleanly, true);
  local_state_->SetInteger(prefs::kStabilityExecutionPhase,
                           MetricsService::SHUTDOWN_COMPLETE);
}

void MetricsService::LogPluginLoadingError(const base::FilePath& plugin_path) {
#if defined(ENABLE_PLUGINS)
  // TODO(asvitkine): Move this out of here.
  plugin_metrics_provider_->LogPluginLoadingError(plugin_path);
#endif
}

bool MetricsService::ShouldLogEvents() {
  // We simply don't log events to UMA if there is a single incognito
  // session visible. The problem is that we always notify using the orginal
  // profile in order to simplify notification processing.
  return !client_->IsOffTheRecordSessionActive();
}

void MetricsService::RecordBooleanPrefValue(const char* path, bool value) {
  DCHECK(IsSingleThreaded());
  local_state_->SetBoolean(path, value);
  RecordCurrentState(local_state_);
}

void MetricsService::RecordCurrentState(PrefService* pref) {
  pref->SetInt64(prefs::kStabilityLastTimestampSec, Time::Now().ToTimeT());

#if defined(ENABLE_PLUGINS)
  plugin_metrics_provider_->RecordPluginChanges();
#endif
}
