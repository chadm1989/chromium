// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/gcm_driver/gcm_client_impl.h"

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "base/metrics/histogram_macros.h"
#include "base/sequenced_task_runner.h"
#include "base/single_thread_task_runner.h"
#include "base/stl_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/thread_task_runner_handle.h"
#include "base/time/default_clock.h"
#include "base/timer/timer.h"
#include "components/gcm_driver/gcm_account_mapper.h"
#include "components/gcm_driver/gcm_backoff_policy.h"
#include "google_apis/gcm/base/encryptor.h"
#include "google_apis/gcm/base/mcs_message.h"
#include "google_apis/gcm/base/mcs_util.h"
#include "google_apis/gcm/engine/checkin_request.h"
#include "google_apis/gcm/engine/connection_factory_impl.h"
#include "google_apis/gcm/engine/gcm_registration_request_handler.h"
#include "google_apis/gcm/engine/gcm_store_impl.h"
#include "google_apis/gcm/engine/gcm_unregistration_request_handler.h"
#include "google_apis/gcm/engine/instance_id_delete_token_request_handler.h"
#include "google_apis/gcm/engine/instance_id_get_token_request_handler.h"
#include "google_apis/gcm/monitoring/gcm_stats_recorder.h"
#include "google_apis/gcm/protocol/checkin.pb.h"
#include "google_apis/gcm/protocol/mcs.pb.h"
#include "net/http/http_network_session.h"
#include "net/http/http_transaction_factory.h"
#include "net/url_request/url_request_context.h"
#include "url/gurl.h"

namespace gcm {

namespace {

// Indicates a message type of the received message.
enum MessageType {
  UNKNOWN,           // Undetermined type.
  DATA_MESSAGE,      // Regular data message.
  DELETED_MESSAGES,  // Messages were deleted on the server.
  SEND_ERROR,        // Error sending a message.
};

enum OutgoingMessageTTLCategory {
  TTL_ZERO,
  TTL_LESS_THAN_OR_EQUAL_TO_ONE_MINUTE,
  TTL_LESS_THAN_OR_EQUAL_TO_ONE_HOUR,
  TTL_LESS_THAN_OR_EQUAL_TO_ONE_DAY,
  TTL_LESS_THAN_OR_EQUAL_TO_ONE_WEEK,
  TTL_MORE_THAN_ONE_WEEK,
  TTL_MAXIMUM,
  // NOTE: always keep this entry at the end. Add new TTL category only
  // immediately above this line. Make sure to update the corresponding
  // histogram enum accordingly.
  TTL_CATEGORY_COUNT
};

enum ResetStoreError {
  DESTROYING_STORE_FAILED,
  INFINITE_STORE_RESET,
  // NOTE: always keep this entry at the end. Add new value only immediately
  // above this line. Make sure to update the corresponding histogram enum
  // accordingly.
  RESET_STORE_ERROR_COUNT
};

const char kGCMScope[] = "GCM";
const int kMaxRegistrationRetries = 5;
const char kMessageTypeDataMessage[] = "gcm";
const char kMessageTypeDeletedMessagesKey[] = "deleted_messages";
const char kMessageTypeKey[] = "message_type";
const char kMessageTypeSendErrorKey[] = "send_error";
const char kSendErrorMessageIdKey[] = "google.message_id";
const char kSendMessageFromValue[] = "gcm@chrome.com";
const int64 kDefaultUserSerialNumber = 0LL;

GCMClient::Result ToGCMClientResult(MCSClient::MessageSendStatus status) {
  switch (status) {
    case MCSClient::QUEUED:
      return GCMClient::SUCCESS;
    case MCSClient::QUEUE_SIZE_LIMIT_REACHED:
      return GCMClient::NETWORK_ERROR;
    case MCSClient::APP_QUEUE_SIZE_LIMIT_REACHED:
      return GCMClient::NETWORK_ERROR;
    case MCSClient::MESSAGE_TOO_LARGE:
      return GCMClient::INVALID_PARAMETER;
    case MCSClient::NO_CONNECTION_ON_ZERO_TTL:
      return GCMClient::NETWORK_ERROR;
    case MCSClient::TTL_EXCEEDED:
      return GCMClient::NETWORK_ERROR;
    case MCSClient::SENT:
    default:
      NOTREACHED();
      break;
  }
  return GCMClientImpl::UNKNOWN_ERROR;
}

void ToCheckinProtoVersion(
    const GCMClient::ChromeBuildInfo& chrome_build_info,
    checkin_proto::ChromeBuildProto* android_build_info) {
  checkin_proto::ChromeBuildProto_Platform platform;
  switch (chrome_build_info.platform) {
    case GCMClient::PLATFORM_WIN:
      platform = checkin_proto::ChromeBuildProto_Platform_PLATFORM_WIN;
      break;
    case GCMClient::PLATFORM_MAC:
      platform = checkin_proto::ChromeBuildProto_Platform_PLATFORM_MAC;
      break;
    case GCMClient::PLATFORM_LINUX:
      platform = checkin_proto::ChromeBuildProto_Platform_PLATFORM_LINUX;
      break;
    case GCMClient::PLATFORM_IOS:
      platform = checkin_proto::ChromeBuildProto_Platform_PLATFORM_IOS;
      break;
    case GCMClient::PLATFORM_ANDROID:
      platform = checkin_proto::ChromeBuildProto_Platform_PLATFORM_ANDROID;
      break;
    case GCMClient::PLATFORM_CROS:
      platform = checkin_proto::ChromeBuildProto_Platform_PLATFORM_CROS;
      break;
    case GCMClient::PLATFORM_UNKNOWN:
      // For unknown platform, return as LINUX.
      platform = checkin_proto::ChromeBuildProto_Platform_PLATFORM_LINUX;
      break;
    default:
      NOTREACHED();
      platform = checkin_proto::ChromeBuildProto_Platform_PLATFORM_LINUX;
      break;
  }
  android_build_info->set_platform(platform);

  checkin_proto::ChromeBuildProto_Channel channel;
  switch (chrome_build_info.channel) {
    case GCMClient::CHANNEL_STABLE:
      channel = checkin_proto::ChromeBuildProto_Channel_CHANNEL_STABLE;
      break;
    case GCMClient::CHANNEL_BETA:
      channel = checkin_proto::ChromeBuildProto_Channel_CHANNEL_BETA;
      break;
    case GCMClient::CHANNEL_DEV:
      channel = checkin_proto::ChromeBuildProto_Channel_CHANNEL_DEV;
      break;
    case GCMClient::CHANNEL_CANARY:
      channel = checkin_proto::ChromeBuildProto_Channel_CHANNEL_CANARY;
      break;
    case GCMClient::CHANNEL_UNKNOWN:
      channel = checkin_proto::ChromeBuildProto_Channel_CHANNEL_UNKNOWN;
      break;
    default:
      NOTREACHED();
      channel = checkin_proto::ChromeBuildProto_Channel_CHANNEL_UNKNOWN;
      break;
  }
  android_build_info->set_channel(channel);

  android_build_info->set_chrome_version(chrome_build_info.version);
}

MessageType DecodeMessageType(const std::string& value) {
  if (kMessageTypeDeletedMessagesKey == value)
    return DELETED_MESSAGES;
  if (kMessageTypeSendErrorKey == value)
    return SEND_ERROR;
  if (kMessageTypeDataMessage == value)
    return DATA_MESSAGE;
  return UNKNOWN;
}

int ConstructGCMVersion(const std::string& chrome_version) {
  // Major Chrome version is passed as GCM version.
  size_t pos = chrome_version.find('.');
  if (pos == std::string::npos) {
    NOTREACHED();
    return 0;
  }

  int gcm_version = 0;
  base::StringToInt(
      base::StringPiece(chrome_version.c_str(), pos), &gcm_version);
  return gcm_version;
}

std::string SerializeInstanceIDData(const std::string& instance_id,
                                    const std::string& extra_data) {
  DCHECK(!instance_id.empty() && !extra_data.empty());
  DCHECK(instance_id.find(',') == std::string::npos);
  return instance_id + "," + extra_data;
}

bool DeserializeInstanceIDData(const std::string& serialized_data,
                               std::string* instance_id,
                               std::string* extra_data) {
  DCHECK(instance_id && extra_data);
  std::size_t pos = serialized_data.find(',');
  if (pos == std::string::npos)
    return false;
  *instance_id = serialized_data.substr(0, pos);
  *extra_data = serialized_data.substr(pos + 1);
  return !instance_id->empty() && !extra_data->empty();
}

void RecordOutgoingMessageToUMA(
    const gcm::GCMClient::OutgoingMessage& message) {
  OutgoingMessageTTLCategory ttl_category;
  if (message.time_to_live == 0)
    ttl_category = TTL_ZERO;
  else if (message.time_to_live <= 60 )
    ttl_category = TTL_LESS_THAN_OR_EQUAL_TO_ONE_MINUTE;
  else if (message.time_to_live <= 60 * 60)
    ttl_category = TTL_LESS_THAN_OR_EQUAL_TO_ONE_HOUR;
  else if (message.time_to_live <= 24 * 60 * 60)
    ttl_category = TTL_LESS_THAN_OR_EQUAL_TO_ONE_DAY;
  else
    ttl_category = TTL_MAXIMUM;

  UMA_HISTOGRAM_ENUMERATION("GCM.OutgoingMessageTTL",
                            ttl_category,
                            TTL_CATEGORY_COUNT);
}

void RecordResetStoreErrorToUMA(ResetStoreError error) {
  UMA_HISTOGRAM_ENUMERATION("GCM.ResetStore", error, RESET_STORE_ERROR_COUNT);
}

}  // namespace

GCMInternalsBuilder::GCMInternalsBuilder() {}
GCMInternalsBuilder::~GCMInternalsBuilder() {}

scoped_ptr<base::Clock> GCMInternalsBuilder::BuildClock() {
  return make_scoped_ptr<base::Clock>(new base::DefaultClock());
}

scoped_ptr<MCSClient> GCMInternalsBuilder::BuildMCSClient(
    const std::string& version,
    base::Clock* clock,
    ConnectionFactory* connection_factory,
    GCMStore* gcm_store,
    GCMStatsRecorder* recorder) {
  return scoped_ptr<MCSClient>(new MCSClient(
      version, clock, connection_factory, gcm_store, recorder));
}

scoped_ptr<ConnectionFactory> GCMInternalsBuilder::BuildConnectionFactory(
      const std::vector<GURL>& endpoints,
      const net::BackoffEntry::Policy& backoff_policy,
      const scoped_refptr<net::HttpNetworkSession>& gcm_network_session,
      const scoped_refptr<net::HttpNetworkSession>& http_network_session,
      net::NetLog* net_log,
      GCMStatsRecorder* recorder) {
  return make_scoped_ptr<ConnectionFactory>(
      new ConnectionFactoryImpl(endpoints,
                                backoff_policy,
                                gcm_network_session,
                                http_network_session,
                                net_log,
                                recorder));
}

GCMClientImpl::CheckinInfo::CheckinInfo()
    : android_id(0), secret(0), accounts_set(false) {
}

GCMClientImpl::CheckinInfo::~CheckinInfo() {
}

void GCMClientImpl::CheckinInfo::SnapshotCheckinAccounts() {
  last_checkin_accounts.clear();
  for (std::map<std::string, std::string>::iterator iter =
           account_tokens.begin();
       iter != account_tokens.end();
       ++iter) {
    last_checkin_accounts.insert(iter->first);
  }
}

void GCMClientImpl::CheckinInfo::Reset() {
  android_id = 0;
  secret = 0;
  accounts_set = false;
  account_tokens.clear();
  last_checkin_accounts.clear();
}

GCMClientImpl::GCMClientImpl(scoped_ptr<GCMInternalsBuilder> internals_builder)
    : internals_builder_(internals_builder.Pass()),
      state_(UNINITIALIZED),
      delegate_(NULL),
      start_mode_(DELAYED_START),
      clock_(internals_builder_->BuildClock()),
      gcm_store_reset_(false),
      url_request_context_getter_(NULL),
      pending_registration_requests_deleter_(&pending_registration_requests_),
      pending_unregistration_requests_deleter_(
          &pending_unregistration_requests_),
      periodic_checkin_ptr_factory_(this),
      weak_ptr_factory_(this) {
}

GCMClientImpl::~GCMClientImpl() {
}

void GCMClientImpl::Initialize(
    const ChromeBuildInfo& chrome_build_info,
    const base::FilePath& path,
    const scoped_refptr<base::SequencedTaskRunner>& blocking_task_runner,
    const scoped_refptr<net::URLRequestContextGetter>&
        url_request_context_getter,
    scoped_ptr<Encryptor> encryptor,
    GCMClient::Delegate* delegate) {
  DCHECK_EQ(UNINITIALIZED, state_);
  DCHECK(url_request_context_getter.get());
  DCHECK(delegate);

  url_request_context_getter_ = url_request_context_getter;
  const net::HttpNetworkSession::Params* network_session_params =
      url_request_context_getter_->GetURLRequestContext()->
          GetNetworkSessionParams();
  DCHECK(network_session_params);
  network_session_ = new net::HttpNetworkSession(*network_session_params);

  chrome_build_info_ = chrome_build_info;

  gcm_store_.reset(
      new GCMStoreImpl(path, blocking_task_runner, encryptor.Pass()));

  delegate_ = delegate;

  recorder_.SetDelegate(this);

  state_ = INITIALIZED;
}

void GCMClientImpl::Start(StartMode start_mode) {
  DCHECK_NE(UNINITIALIZED, state_);

  if (state_ == LOADED) {
    // Start the GCM if not yet.
    if (start_mode == IMMEDIATE_START)
      StartGCM();
    return;
  }

  // The delay start behavior will be abandoned when Start has been called
  // once with IMMEDIATE_START behavior.
  if (start_mode == IMMEDIATE_START)
    start_mode_ = IMMEDIATE_START;

  // Bail out if the loading is not started or completed.
  if (state_ != INITIALIZED)
    return;

  // Once the loading is completed, the check-in will be initiated.
  // If we're in lazy start mode, don't create a new store since none is really
  // using GCM functionality yet.
  gcm_store_->Load(
      (start_mode == IMMEDIATE_START) ?
          GCMStore::CREATE_IF_MISSING :
          GCMStore::DO_NOT_CREATE,
      base::Bind(&GCMClientImpl::OnLoadCompleted,
                 weak_ptr_factory_.GetWeakPtr()));
  state_ = LOADING;
}

void GCMClientImpl::OnLoadCompleted(scoped_ptr<GCMStore::LoadResult> result) {
  DCHECK_EQ(LOADING, state_);

  if (!result->success) {
    if (result->store_does_not_exist) {
      // In the case that the store does not exist, set |state| back to
      // INITIALIZED such that store loading could be triggered again when
      // Start() is called with IMMEDIATE_START.
      state_ = INITIALIZED;
    } else {
      // Otherwise, destroy the store to try again.
      ResetStore();
    }
    return;
  }
  gcm_store_reset_ = false;

  device_checkin_info_.android_id = result->device_android_id;
  device_checkin_info_.secret = result->device_security_token;
  device_checkin_info_.last_checkin_accounts = result->last_checkin_accounts;
  // A case where there were previously no accounts reported with checkin is
  // considered to be the same as when the list of accounts is empty. It enables
  // scheduling a periodic checkin for devices with no signed in users
  // immediately after restart, while keeping |accounts_set == false| delays the
  // checkin until the list of accounts is set explicitly.
  if (result->last_checkin_accounts.size() == 0)
    device_checkin_info_.accounts_set = true;
  last_checkin_time_ = result->last_checkin_time;
  gservices_settings_.UpdateFromLoadResult(*result);

  for (auto iter = result->registrations.begin();
       iter != result->registrations.end();
       ++iter) {
    std::string registration_id;
    scoped_ptr<RegistrationInfo> registration =
        RegistrationInfo::BuildFromString(
            iter->first, iter->second, &registration_id);
    // TODO(jianli): Add UMA to track the error case.
    if (registration.get())
      registrations_[make_linked_ptr(registration.release())] = registration_id;
  }

  for (auto iter = result->instance_id_data.begin();
       iter != result->instance_id_data.end();
       ++iter) {
    std::string instance_id;
    std::string extra_data;
    if (DeserializeInstanceIDData(iter->second, &instance_id, &extra_data))
      instance_id_data_[iter->first] = std::make_pair(instance_id, extra_data);
  }

  load_result_ = result.Pass();
  state_ = LOADED;

  // Don't initiate the GCM connection when GCM is in delayed start mode and
  // not any standalone app has registered GCM yet.
  if (start_mode_ == DELAYED_START && !HasStandaloneRegisteredApp())
    return;

  StartGCM();
}

void GCMClientImpl::StartGCM() {
  // Taking over the value of account_mappings before passing the ownership of
  // load result to InitializeMCSClient.
  std::vector<AccountMapping> account_mappings;
  account_mappings.swap(load_result_->account_mappings);
  base::Time last_token_fetch_time = load_result_->last_token_fetch_time;

  InitializeMCSClient();

  if (device_checkin_info_.IsValid()) {
    SchedulePeriodicCheckin();
    OnReady(account_mappings, last_token_fetch_time);
    return;
  }

  state_ = INITIAL_DEVICE_CHECKIN;
  device_checkin_info_.Reset();
  StartCheckin();
}

void GCMClientImpl::InitializeMCSClient() {
  std::vector<GURL> endpoints;
  endpoints.push_back(gservices_settings_.GetMCSMainEndpoint());
  endpoints.push_back(gservices_settings_.GetMCSFallbackEndpoint());
  connection_factory_ = internals_builder_->BuildConnectionFactory(
      endpoints,
      GetGCMBackoffPolicy(),
      network_session_,
      url_request_context_getter_->GetURLRequestContext()
          ->http_transaction_factory()
          ->GetSession(),
      net_log_.net_log(),
      &recorder_);
  connection_factory_->SetConnectionListener(this);
  mcs_client_ = internals_builder_->BuildMCSClient(
      chrome_build_info_.version,
      clock_.get(),
      connection_factory_.get(),
      gcm_store_.get(),
      &recorder_).Pass();

  mcs_client_->Initialize(
      base::Bind(&GCMClientImpl::OnMCSError, weak_ptr_factory_.GetWeakPtr()),
      base::Bind(&GCMClientImpl::OnMessageReceivedFromMCS,
                 weak_ptr_factory_.GetWeakPtr()),
      base::Bind(&GCMClientImpl::OnMessageSentToMCS,
                 weak_ptr_factory_.GetWeakPtr()),
      load_result_.Pass());
}

void GCMClientImpl::OnFirstTimeDeviceCheckinCompleted(
    const CheckinInfo& checkin_info) {
  DCHECK(!device_checkin_info_.IsValid());

  device_checkin_info_.android_id = checkin_info.android_id;
  device_checkin_info_.secret = checkin_info.secret;
  // If accounts were not set by now, we can consider them set (to empty list)
  // to make sure periodic checkins get scheduled after initial checkin.
  device_checkin_info_.accounts_set = true;
  gcm_store_->SetDeviceCredentials(
      checkin_info.android_id, checkin_info.secret,
      base::Bind(&GCMClientImpl::SetDeviceCredentialsCallback,
                 weak_ptr_factory_.GetWeakPtr()));

  OnReady(std::vector<AccountMapping>(), base::Time());
}

void GCMClientImpl::OnReady(const std::vector<AccountMapping>& account_mappings,
                            const base::Time& last_token_fetch_time) {
  state_ = READY;
  StartMCSLogin();

  delegate_->OnGCMReady(account_mappings, last_token_fetch_time);
}

void GCMClientImpl::StartMCSLogin() {
  DCHECK_EQ(READY, state_);
  DCHECK(device_checkin_info_.IsValid());
  mcs_client_->Login(device_checkin_info_.android_id,
                     device_checkin_info_.secret);
}

void GCMClientImpl::ResetStore() {
  DCHECK_EQ(LOADING, state_);

  // If already being reset, don't do it again. We want to prevent from
  // resetting and loading from the store again and again.
  if (gcm_store_reset_) {
    RecordResetStoreErrorToUMA(INFINITE_STORE_RESET);
    state_ = UNINITIALIZED;
    return;
  }
  gcm_store_reset_ = true;

  // Destroy the GCM store to start over.
  gcm_store_->Destroy(base::Bind(&GCMClientImpl::ResetStoreCallback,
                      weak_ptr_factory_.GetWeakPtr()));
}

void GCMClientImpl::SetAccountTokens(
    const std::vector<AccountTokenInfo>& account_tokens) {
  device_checkin_info_.account_tokens.clear();
  for (std::vector<AccountTokenInfo>::const_iterator iter =
           account_tokens.begin();
       iter != account_tokens.end();
       ++iter) {
    device_checkin_info_.account_tokens[iter->email] = iter->access_token;
  }

  bool accounts_set_before = device_checkin_info_.accounts_set;
  device_checkin_info_.accounts_set = true;

  DVLOG(1) << "Set account called with: " << account_tokens.size()
           << " accounts.";

  if (state_ != READY && state_ != INITIAL_DEVICE_CHECKIN)
    return;

  bool account_removed = false;
  for (std::set<std::string>::iterator iter =
           device_checkin_info_.last_checkin_accounts.begin();
       iter != device_checkin_info_.last_checkin_accounts.end();
       ++iter) {
    if (device_checkin_info_.account_tokens.find(*iter) ==
            device_checkin_info_.account_tokens.end()) {
      account_removed = true;
    }
  }

  // Checkin will be forced when any of the accounts was removed during the
  // current Chrome session or if there has been an account removed between the
  // restarts of Chrome. If there is a checkin in progress, it will be canceled.
  // We only force checkin when user signs out. When there is a new account
  // signed in, the periodic checkin will take care of adding the association in
  // reasonable time.
  if (account_removed) {
    DVLOG(1) << "Detected that account has been removed. Forcing checkin.";
    checkin_request_.reset();
    StartCheckin();
  } else if (!accounts_set_before) {
    SchedulePeriodicCheckin();
    DVLOG(1) << "Accounts set for the first time. Scheduled periodic checkin.";
  }
}

void GCMClientImpl::UpdateAccountMapping(
    const AccountMapping& account_mapping) {
  gcm_store_->AddAccountMapping(account_mapping,
                                base::Bind(&GCMClientImpl::DefaultStoreCallback,
                                           weak_ptr_factory_.GetWeakPtr()));
}

void GCMClientImpl::RemoveAccountMapping(const std::string& account_id) {
  gcm_store_->RemoveAccountMapping(
      account_id,
      base::Bind(&GCMClientImpl::DefaultStoreCallback,
                 weak_ptr_factory_.GetWeakPtr()));
}

void GCMClientImpl::SetLastTokenFetchTime(const base::Time& time) {
  gcm_store_->SetLastTokenFetchTime(
      time,
      base::Bind(&GCMClientImpl::IgnoreWriteResultCallback,
                 weak_ptr_factory_.GetWeakPtr()));
}

void GCMClientImpl::UpdateHeartbeatTimer(scoped_ptr<base::Timer> timer) {
  DCHECK(mcs_client_);
  mcs_client_->UpdateHeartbeatTimer(timer.Pass());
}

void GCMClientImpl::AddInstanceIDData(const std::string& app_id,
                                      const std::string& instance_id,
                                      const std::string& extra_data) {
  instance_id_data_[app_id] = std::make_pair(instance_id, extra_data);
  gcm_store_->AddInstanceIDData(
      app_id,
      SerializeInstanceIDData(instance_id, extra_data),
      base::Bind(&GCMClientImpl::IgnoreWriteResultCallback,
                 weak_ptr_factory_.GetWeakPtr()));
}

void GCMClientImpl::RemoveInstanceIDData(const std::string& app_id) {
  instance_id_data_.erase(app_id);
  gcm_store_->RemoveInstanceIDData(
      app_id,
      base::Bind(&GCMClientImpl::IgnoreWriteResultCallback,
                 weak_ptr_factory_.GetWeakPtr()));
}

void GCMClientImpl::GetInstanceIDData(const std::string& app_id,
                                      std::string* instance_id,
                                      std::string* extra_data) {
  DCHECK(instance_id && extra_data);

  auto iter = instance_id_data_.find(app_id);
  if (iter == instance_id_data_.end())
    return;
  *instance_id = iter->second.first;
  *extra_data = iter->second.second;
}

void GCMClientImpl::AddHeartbeatInterval(const std::string& scope,
                                         int interval_ms) {
  DCHECK(mcs_client_);
  mcs_client_->AddHeartbeatInterval(scope, interval_ms);
}

void GCMClientImpl::RemoveHeartbeatInterval(const std::string& scope) {
  DCHECK(mcs_client_);
  mcs_client_->RemoveHeartbeatInterval(scope);
}

void GCMClientImpl::StartCheckin() {
  // Make sure no checkin is in progress.
  if (checkin_request_.get())
    return;

  checkin_proto::ChromeBuildProto chrome_build_proto;
  ToCheckinProtoVersion(chrome_build_info_, &chrome_build_proto);
  CheckinRequest::RequestInfo request_info(device_checkin_info_.android_id,
                                           device_checkin_info_.secret,
                                           device_checkin_info_.account_tokens,
                                           gservices_settings_.digest(),
                                           chrome_build_proto);
  checkin_request_.reset(
      new CheckinRequest(gservices_settings_.GetCheckinURL(),
                         request_info,
                         GetGCMBackoffPolicy(),
                         base::Bind(&GCMClientImpl::OnCheckinCompleted,
                                    weak_ptr_factory_.GetWeakPtr()),
                         url_request_context_getter_.get(),
                         &recorder_));
  // Taking a snapshot of the accounts count here, as there might be an asynch
  // update of the account tokens while checkin is in progress.
  device_checkin_info_.SnapshotCheckinAccounts();
  checkin_request_->Start();
}

void GCMClientImpl::OnCheckinCompleted(
    const checkin_proto::AndroidCheckinResponse& checkin_response) {
  checkin_request_.reset();

  if (!checkin_response.has_android_id() ||
      !checkin_response.has_security_token()) {
    // TODO(fgorski): I don't think a retry here will help, we should probably
    // start over. By checking in with (0, 0).
    return;
  }

  CheckinInfo checkin_info;
  checkin_info.android_id = checkin_response.android_id();
  checkin_info.secret = checkin_response.security_token();

  if (state_ == INITIAL_DEVICE_CHECKIN) {
    OnFirstTimeDeviceCheckinCompleted(checkin_info);
  } else {
    // checkin_info is not expected to change after a periodic checkin as it
    // would invalidate the registratoin IDs.
    DCHECK_EQ(READY, state_);
    DCHECK_EQ(device_checkin_info_.android_id, checkin_info.android_id);
    DCHECK_EQ(device_checkin_info_.secret, checkin_info.secret);
  }

  if (device_checkin_info_.IsValid()) {
    // First update G-services settings, as something might have changed.
    if (gservices_settings_.UpdateFromCheckinResponse(checkin_response)) {
      gcm_store_->SetGServicesSettings(
          gservices_settings_.settings_map(),
          gservices_settings_.digest(),
          base::Bind(&GCMClientImpl::SetGServicesSettingsCallback,
                     weak_ptr_factory_.GetWeakPtr()));
    }

    last_checkin_time_ = clock_->Now();
    gcm_store_->SetLastCheckinInfo(
        last_checkin_time_,
        device_checkin_info_.last_checkin_accounts,
        base::Bind(&GCMClientImpl::SetLastCheckinInfoCallback,
                   weak_ptr_factory_.GetWeakPtr()));
    SchedulePeriodicCheckin();
  }
}

void GCMClientImpl::SetGServicesSettingsCallback(bool success) {
  DCHECK(success);
}

void GCMClientImpl::SchedulePeriodicCheckin() {
  // Make sure no checkin is in progress.
  if (checkin_request_.get() || !device_checkin_info_.accounts_set)
    return;

  // There should be only one periodic checkin pending at a time. Removing
  // pending periodic checkin to schedule a new one.
  periodic_checkin_ptr_factory_.InvalidateWeakPtrs();

  base::TimeDelta time_to_next_checkin = GetTimeToNextCheckin();
  if (time_to_next_checkin < base::TimeDelta())
    time_to_next_checkin = base::TimeDelta();

  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE, base::Bind(&GCMClientImpl::StartCheckin,
                            periodic_checkin_ptr_factory_.GetWeakPtr()),
      time_to_next_checkin);
}

base::TimeDelta GCMClientImpl::GetTimeToNextCheckin() const {
  return last_checkin_time_ + gservices_settings_.GetCheckinInterval() -
         clock_->Now();
}

void GCMClientImpl::SetLastCheckinInfoCallback(bool success) {
  // TODO(fgorski): This is one of the signals that store needs a rebuild.
  DCHECK(success);
}

void GCMClientImpl::SetDeviceCredentialsCallback(bool success) {
  // TODO(fgorski): This is one of the signals that store needs a rebuild.
  DCHECK(success);
}

void GCMClientImpl::UpdateRegistrationCallback(bool success) {
  // TODO(fgorski): This is one of the signals that store needs a rebuild.
  DCHECK(success);
}

void GCMClientImpl::DefaultStoreCallback(bool success) {
  DCHECK(success);
}

void GCMClientImpl::IgnoreWriteResultCallback(bool success) {
  // TODO(fgorski): Ignoring the write result for now to make sure
  // sync_intergration_tests are not broken.
}

void GCMClientImpl::ResetStoreCallback(bool success) {
  if (!success) {
    LOG(ERROR) << "Failed to reset GCM store";
    RecordResetStoreErrorToUMA(DESTROYING_STORE_FAILED);
    state_ = UNINITIALIZED;
    return;
  }

  state_ = INITIALIZED;
  Start(start_mode_);
}

void GCMClientImpl::Stop() {
  // TODO(fgorski): Perhaps we should make a distinction between a Stop and a
  // Shutdown.
  DVLOG(1) << "Stopping the GCM Client";
  weak_ptr_factory_.InvalidateWeakPtrs();
  periodic_checkin_ptr_factory_.InvalidateWeakPtrs();
  device_checkin_info_.Reset();
  connection_factory_.reset();
  delegate_->OnDisconnected();
  mcs_client_.reset();
  checkin_request_.reset();
  // Delete all of the pending registration and unregistration requests.
  STLDeleteValues(&pending_registration_requests_);
  STLDeleteValues(&pending_unregistration_requests_);
  state_ = INITIALIZED;
  gcm_store_->Close();
}

void GCMClientImpl::Register(
    const linked_ptr<RegistrationInfo>& registration_info) {
  DCHECK_EQ(state_, READY);

  // Find and use the cached registration ID.
  RegistrationInfoMap::const_iterator registrations_iter =
      registrations_.find(registration_info);
  if (registrations_iter != registrations_.end()) {
    bool matched = true;

    // For GCM registration, we also match the sender IDs since multiple
    // registrations are not supported.
    const GCMRegistrationInfo* gcm_registration_info =
        GCMRegistrationInfo::FromRegistrationInfo(registration_info.get());
    if (gcm_registration_info) {
      const GCMRegistrationInfo* cached_gcm_registration_info =
          GCMRegistrationInfo::FromRegistrationInfo(
              registrations_iter->first.get());
      DCHECK(cached_gcm_registration_info);
      if (cached_gcm_registration_info &&
          gcm_registration_info->sender_ids !=
              cached_gcm_registration_info->sender_ids) {
        matched = false;
      }
    }

    if (matched) {
      delegate_->OnRegisterFinished(
          registration_info, registrations_iter->second, SUCCESS);
      return;
    }
  }

  scoped_ptr<RegistrationRequest::CustomRequestHandler> request_handler;
  std::string source_to_record;

  const GCMRegistrationInfo* gcm_registration_info =
      GCMRegistrationInfo::FromRegistrationInfo(registration_info.get());
  if (gcm_registration_info) {
    std::string senders;
    for (auto iter = gcm_registration_info->sender_ids.begin();
         iter != gcm_registration_info->sender_ids.end();
         ++iter) {
      if (!senders.empty())
        senders.append(",");
      senders.append(*iter);
    }
    UMA_HISTOGRAM_COUNTS("GCM.RegistrationSenderIdCount",
                         gcm_registration_info->sender_ids.size());

    request_handler.reset(new GCMRegistrationRequestHandler(senders));
    source_to_record = senders;
  }

  const InstanceIDTokenInfo* instance_id_token_info =
      InstanceIDTokenInfo::FromRegistrationInfo(registration_info.get());
  if (instance_id_token_info) {
    auto instance_id_iter = instance_id_data_.find(registration_info->app_id);
    DCHECK(instance_id_iter != instance_id_data_.end());

    request_handler.reset(new InstanceIDGetTokenRequestHandler(
          instance_id_iter->second.first,
          instance_id_token_info->authorized_entity,
          instance_id_token_info->scope,
          ConstructGCMVersion(chrome_build_info_.version),
          instance_id_token_info->options));
    source_to_record = instance_id_token_info->authorized_entity;
  }

  RegistrationRequest::RequestInfo request_info(
      device_checkin_info_.android_id,
      device_checkin_info_.secret,
      registration_info->app_id);

  RegistrationRequest* registration_request =
      new RegistrationRequest(gservices_settings_.GetRegistrationURL(),
                              request_info,
                              request_handler.Pass(),
                              GetGCMBackoffPolicy(),
                              base::Bind(&GCMClientImpl::OnRegisterCompleted,
                                          weak_ptr_factory_.GetWeakPtr(),
                                          registration_info),
                              kMaxRegistrationRetries,
                              url_request_context_getter_,
                              &recorder_,
                              source_to_record);
  pending_registration_requests_[registration_info] = registration_request;
  registration_request->Start();
}

void GCMClientImpl::OnRegisterCompleted(
    const linked_ptr<RegistrationInfo>& registration_info,
    RegistrationRequest::Status status,
    const std::string& registration_id) {
  DCHECK(delegate_);

  Result result;
  PendingRegistrationRequests::iterator iter =
      pending_registration_requests_.find(registration_info);
  if (iter == pending_registration_requests_.end())
    result = UNKNOWN_ERROR;
  else if (status == RegistrationRequest::INVALID_SENDER)
    result = INVALID_PARAMETER;
  else if (registration_id.empty())
    result = SERVER_ERROR;
  else
    result = SUCCESS;

  if (result == SUCCESS) {
    // Cache it.
    registrations_[registration_info] = registration_id;

    // Save it in the persistent store.
    gcm_store_->AddRegistration(
        registration_info->GetSerializedKey(),
        registration_info->GetSerializedValue(registration_id),
        base::Bind(&GCMClientImpl::UpdateRegistrationCallback,
                   weak_ptr_factory_.GetWeakPtr()));
  }

  delegate_->OnRegisterFinished(
      registration_info,
      result == SUCCESS ? registration_id : std::string(),
      result);

  if (iter != pending_registration_requests_.end()) {
    delete iter->second;
    pending_registration_requests_.erase(iter);
  }
}

void GCMClientImpl::Unregister(
    const linked_ptr<RegistrationInfo>& registration_info) {
  DCHECK_EQ(state_, READY);
  if (pending_unregistration_requests_.count(registration_info) == 1)
    return;

  // Remove from the cache and persistent store.
  registrations_.erase(registration_info);
  gcm_store_->RemoveRegistration(
      registration_info->GetSerializedKey(),
      base::Bind(&GCMClientImpl::UpdateRegistrationCallback,
                 weak_ptr_factory_.GetWeakPtr()));

  scoped_ptr<UnregistrationRequest::CustomRequestHandler> request_handler;

  const GCMRegistrationInfo* gcm_registration_info =
      GCMRegistrationInfo::FromRegistrationInfo(registration_info.get());
  if (gcm_registration_info) {
    request_handler.reset(
        new GCMUnregistrationRequestHandler(registration_info->app_id));
  }

  const InstanceIDTokenInfo* instance_id_token_info =
      InstanceIDTokenInfo::FromRegistrationInfo(registration_info.get());
  if (instance_id_token_info) {
    auto instance_id_iter = instance_id_data_.find(registration_info->app_id);
    if (instance_id_iter == instance_id_data_.end()) {
      // This should not be reached since we should not delete tokens when
      // an InstanceID has not been created yet.
      NOTREACHED();
      return;
    }

    std::string instance_id = instance_id_iter->second.first;
    std::string app_id = instance_id_token_info->app_id;

    // Removes all tokens associated with the app id when authorized_entity
    // and scope are set to '*'.
    if (instance_id_token_info->authorized_entity == "*" &&
        instance_id_token_info->scope == "*") {
      for (auto iter = registrations_.begin();
           iter != registrations_.end();) {
        InstanceIDTokenInfo* cached_instance_id_token_info =
            InstanceIDTokenInfo::FromRegistrationInfo(iter->first.get());
        if (cached_instance_id_token_info &&
            cached_instance_id_token_info->app_id == app_id) {
          gcm_store_->RemoveRegistration(
              cached_instance_id_token_info->GetSerializedKey(),
              base::Bind(&GCMClientImpl::UpdateRegistrationCallback,
                         weak_ptr_factory_.GetWeakPtr()));
          registrations_.erase(iter++);
        } else {
          ++iter;
        }
      }
    }

    request_handler.reset(new InstanceIDDeleteTokenRequestHandler(
        instance_id,
        instance_id_token_info->authorized_entity,
        instance_id_token_info->scope,
        ConstructGCMVersion(chrome_build_info_.version)));
  }

  UnregistrationRequest::RequestInfo request_info(
      device_checkin_info_.android_id,
      device_checkin_info_.secret,
      registration_info->app_id);

  UnregistrationRequest* unregistration_request = new UnregistrationRequest(
      gservices_settings_.GetRegistrationURL(),
      request_info,
      request_handler.Pass(),
      GetGCMBackoffPolicy(),
      base::Bind(&GCMClientImpl::OnUnregisterCompleted,
                  weak_ptr_factory_.GetWeakPtr(),
                  registration_info),
      url_request_context_getter_,
      &recorder_);
  pending_unregistration_requests_[registration_info] = unregistration_request;
  unregistration_request->Start();
}

void GCMClientImpl::OnUnregisterCompleted(
    const linked_ptr<RegistrationInfo>& registration_info,
    UnregistrationRequest::Status status) {
  DVLOG(1) << "Unregister completed for app: " << registration_info->app_id
           << " with " << (status ? "success." : "failure.");
  delegate_->OnUnregisterFinished(
      registration_info,
      status == UnregistrationRequest::SUCCESS ? SUCCESS : SERVER_ERROR);

  PendingUnregistrationRequests::iterator iter =
      pending_unregistration_requests_.find(registration_info);
  if (iter == pending_unregistration_requests_.end())
    return;

  delete iter->second;
  pending_unregistration_requests_.erase(iter);
}

void GCMClientImpl::OnGCMStoreDestroyed(bool success) {
  DLOG_IF(ERROR, !success) << "GCM store failed to be destroyed!";
  UMA_HISTOGRAM_BOOLEAN("GCM.StoreDestroySucceeded", success);
}

void GCMClientImpl::Send(const std::string& app_id,
                         const std::string& receiver_id,
                         const OutgoingMessage& message) {
  DCHECK_EQ(state_, READY);

  RecordOutgoingMessageToUMA(message);

  mcs_proto::DataMessageStanza stanza;
  stanza.set_ttl(message.time_to_live);
  stanza.set_sent(clock_->Now().ToInternalValue() /
                  base::Time::kMicrosecondsPerSecond);
  stanza.set_id(message.id);
  stanza.set_from(kSendMessageFromValue);
  stanza.set_to(receiver_id);
  stanza.set_category(app_id);

  for (MessageData::const_iterator iter = message.data.begin();
       iter != message.data.end();
       ++iter) {
    mcs_proto::AppData* app_data = stanza.add_app_data();
    app_data->set_key(iter->first);
    app_data->set_value(iter->second);
  }

  MCSMessage mcs_message(stanza);
  DVLOG(1) << "MCS message size: " << mcs_message.size();
  mcs_client_->SendMessage(mcs_message);
}

std::string GCMClientImpl::GetStateString() const {
  switch(state_) {
    case GCMClientImpl::INITIALIZED:
      return "INITIALIZED";
    case GCMClientImpl::UNINITIALIZED:
      return "UNINITIALIZED";
    case GCMClientImpl::LOADING:
      return "LOADING";
    case GCMClientImpl::LOADED:
      return "LOADED";
    case GCMClientImpl::INITIAL_DEVICE_CHECKIN:
      return "INITIAL_DEVICE_CHECKIN";
    case GCMClientImpl::READY:
      return "READY";
    default:
      NOTREACHED();
      return std::string();
  }
}

void GCMClientImpl::SetRecording(bool recording) {
  recorder_.SetRecording(recording);
}

void GCMClientImpl::ClearActivityLogs() {
  recorder_.Clear();
}

GCMClient::GCMStatistics GCMClientImpl::GetStatistics() const {
  GCMClient::GCMStatistics stats;
  stats.gcm_client_created = true;
  stats.is_recording = recorder_.is_recording();
  stats.gcm_client_state = GetStateString();
  stats.connection_client_created = mcs_client_.get() != NULL;
  if (connection_factory_.get())
    stats.connection_state = connection_factory_->GetConnectionStateString();
  if (mcs_client_.get()) {
    stats.send_queue_size = mcs_client_->GetSendQueueSize();
    stats.resend_queue_size = mcs_client_->GetResendQueueSize();
  }
  if (device_checkin_info_.android_id > 0)
    stats.android_id = device_checkin_info_.android_id;
  recorder_.CollectActivities(&stats.recorded_activities);

  for (RegistrationInfoMap::const_iterator it = registrations_.begin();
       it != registrations_.end(); ++it) {
    stats.registered_app_ids.push_back(it->first->app_id);
  }
  return stats;
}

void GCMClientImpl::OnActivityRecorded() {
  delegate_->OnActivityRecorded();
}

void GCMClientImpl::OnConnected(const GURL& current_server,
                                const net::IPEndPoint& ip_endpoint) {
  // TODO(gcm): expose current server in debug page.
  delegate_->OnActivityRecorded();
  delegate_->OnConnected(ip_endpoint);
}

void GCMClientImpl::OnDisconnected() {
  delegate_->OnActivityRecorded();
  delegate_->OnDisconnected();
}

void GCMClientImpl::OnMessageReceivedFromMCS(const gcm::MCSMessage& message) {
  switch (message.tag()) {
    case kLoginResponseTag:
      DVLOG(1) << "Login response received by GCM Client. Ignoring.";
      return;
    case kDataMessageStanzaTag:
      DVLOG(1) << "A downstream message received. Processing...";
      HandleIncomingMessage(message);
      return;
    default:
      NOTREACHED() << "Message with unexpected tag received by GCMClient";
      return;
  }
}

void GCMClientImpl::OnMessageSentToMCS(int64 user_serial_number,
                                       const std::string& app_id,
                                       const std::string& message_id,
                                       MCSClient::MessageSendStatus status) {
  DCHECK_EQ(user_serial_number, kDefaultUserSerialNumber);
  DCHECK(delegate_);

  // TTL_EXCEEDED is singled out here, because it can happen long time after the
  // message was sent. That is why it comes as |OnMessageSendError| event rather
  // than |OnSendFinished|. SendErrorDetails.additional_data is left empty.
  // All other errors will be raised immediately, through asynchronous callback.
  // It is expected that TTL_EXCEEDED will be issued for a message that was
  // previously issued |OnSendFinished| with status SUCCESS.
  // TODO(jianli): Consider adding UMA for this status.
  if (status == MCSClient::TTL_EXCEEDED) {
    SendErrorDetails send_error_details;
    send_error_details.message_id = message_id;
    send_error_details.result = GCMClient::TTL_EXCEEDED;
    delegate_->OnMessageSendError(app_id, send_error_details);
  } else if (status == MCSClient::SENT) {
    delegate_->OnSendAcknowledged(app_id, message_id);
  } else {
    delegate_->OnSendFinished(app_id, message_id, ToGCMClientResult(status));
  }
}

void GCMClientImpl::OnMCSError() {
  // TODO(fgorski): For now it replaces the initialization method. Long term it
  // should have an error or status passed in.
}

void GCMClientImpl::HandleIncomingMessage(const gcm::MCSMessage& message) {
  DCHECK(delegate_);

  const mcs_proto::DataMessageStanza& data_message_stanza =
      reinterpret_cast<const mcs_proto::DataMessageStanza&>(
          message.GetProtobuf());
  DCHECK_EQ(data_message_stanza.device_user_id(), kDefaultUserSerialNumber);

  // Copying all the data from the stanza to a MessageData object. When present,
  // keys like kMessageTypeKey or kSendErrorMessageIdKey will be filtered out
  // later.
  MessageData message_data;
  for (int i = 0; i < data_message_stanza.app_data_size(); ++i) {
    std::string key = data_message_stanza.app_data(i).key();
    message_data[key] = data_message_stanza.app_data(i).value();
  }

  MessageType message_type = DATA_MESSAGE;
  MessageData::iterator iter = message_data.find(kMessageTypeKey);
  if (iter != message_data.end()) {
    message_type = DecodeMessageType(iter->second);
    message_data.erase(iter);
  }

  switch (message_type) {
    case DATA_MESSAGE:
      HandleIncomingDataMessage(data_message_stanza, message_data);
      break;
    case DELETED_MESSAGES:
      recorder_.RecordDataMessageReceived(data_message_stanza.category(),
                                          data_message_stanza.from(),
                                          data_message_stanza.ByteSize(),
                                          true,
                                          GCMStatsRecorder::DELETED_MESSAGES);
      delegate_->OnMessagesDeleted(data_message_stanza.category());
      break;
    case SEND_ERROR:
      HandleIncomingSendError(data_message_stanza, message_data);
      break;
    case UNKNOWN:
    default:  // Treat default the same as UNKNOWN.
      DVLOG(1) << "Unknown message_type received. Message ignored. "
               << "App ID: " << data_message_stanza.category() << ".";
      break;
  }
}

void GCMClientImpl::HandleIncomingDataMessage(
    const mcs_proto::DataMessageStanza& data_message_stanza,
    MessageData& message_data) {
  std::string app_id = data_message_stanza.category();
  std::string sender = data_message_stanza.from();

  // Drop the message when the app is not registered for the sender of the
  // message.
  bool registered = false;

  // First, find among all GCM registrations.
  scoped_ptr<GCMRegistrationInfo> gcm_registration(new GCMRegistrationInfo);
  gcm_registration->app_id = app_id;
  auto gcm_registration_iter = registrations_.find(
      make_linked_ptr<RegistrationInfo>(gcm_registration.release()));
  if (gcm_registration_iter != registrations_.end()) {
    GCMRegistrationInfo* cached_gcm_registration =
        GCMRegistrationInfo::FromRegistrationInfo(
            gcm_registration_iter->first.get());
    if (cached_gcm_registration &&
        std::find(cached_gcm_registration->sender_ids.begin(),
                  cached_gcm_registration->sender_ids.end(),
                  sender) != cached_gcm_registration->sender_ids.end()) {
      registered = true;
    }
  }

  // Then, find among all InstanceID registrations.
  if (!registered) {
    scoped_ptr<InstanceIDTokenInfo> instance_id_token(new InstanceIDTokenInfo);
    instance_id_token->app_id = app_id;
    instance_id_token->authorized_entity = sender;
    instance_id_token->scope = kGCMScope;
    auto instance_id_token_iter = registrations_.find(
        make_linked_ptr<RegistrationInfo>(instance_id_token.release()));
    if (instance_id_token_iter != registrations_.end())
      registered = true;
  }

  recorder_.RecordDataMessageReceived(app_id, sender,
      data_message_stanza.ByteSize(), registered,
      GCMStatsRecorder::DATA_MESSAGE);
  if (!registered)
    return;

  IncomingMessage incoming_message;
  incoming_message.sender_id = data_message_stanza.from();
  if (data_message_stanza.has_token())
    incoming_message.collapse_key = data_message_stanza.token();
  incoming_message.data = message_data;
  delegate_->OnMessageReceived(app_id, incoming_message);
}

void GCMClientImpl::HandleIncomingSendError(
    const mcs_proto::DataMessageStanza& data_message_stanza,
    MessageData& message_data) {
  SendErrorDetails send_error_details;
  send_error_details.additional_data = message_data;
  send_error_details.result = SERVER_ERROR;

  MessageData::iterator iter =
      send_error_details.additional_data.find(kSendErrorMessageIdKey);
  if (iter != send_error_details.additional_data.end()) {
    send_error_details.message_id = iter->second;
    send_error_details.additional_data.erase(iter);
  }

  recorder_.RecordIncomingSendError(
      data_message_stanza.category(),
      data_message_stanza.to(),
      data_message_stanza.id());
  delegate_->OnMessageSendError(data_message_stanza.category(),
                                send_error_details);
}

bool GCMClientImpl::HasStandaloneRegisteredApp() const {
  if (registrations_.empty())
    return false;
  // Note that account mapper is not counted as a standalone app since it is
  // automatically started when other app uses GCM.
  return registrations_.size() > 1 ||
         !ExistsGCMRegistrationInMap(registrations_, kGCMAccountMapperAppId);
}

}  // namespace gcm
