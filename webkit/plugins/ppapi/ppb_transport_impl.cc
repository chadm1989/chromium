// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webkit/plugins/ppapi/ppb_transport_impl.h"

#include "base/message_loop.h"
#include "base/string_util.h"
#include "net/base/io_buffer.h"
#include "net/base/net_errors.h"
#include "net/socket/socket.h"
#include "ppapi/c/dev/ppb_transport_dev.h"
#include "ppapi/c/pp_completion_callback.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/shared_impl/var.h"
#include "webkit/plugins/ppapi/common.h"
#include "webkit/plugins/ppapi/plugin_module.h"
#include "webkit/plugins/ppapi/ppapi_plugin_instance.h"

using ppapi::StringVar;
using ppapi::thunk::PPB_Transport_API;
using webkit_glue::P2PTransport;

namespace webkit {
namespace ppapi {

namespace {

const char kUdpProtocolName[] = "udp";
const char kTcpProtocolName[] = "tcp";

int MapNetError(int result) {
  if (result > 0)
    return result;

  switch (result) {
    case net::OK:
      return PP_OK;
    case net::ERR_IO_PENDING:
      return PP_OK_COMPLETIONPENDING;
    case net::ERR_INVALID_ARGUMENT:
      return PP_ERROR_BADARGUMENT;
    default:
      return PP_ERROR_FAILED;
  }
}

}  // namespace

PPB_Transport_Impl::PPB_Transport_Impl(PluginInstance* instance)
    : Resource(instance),
      started_(false),
      writable_(false),
      ALLOW_THIS_IN_INITIALIZER_LIST(
          channel_write_callback_(this, &PPB_Transport_Impl::OnWritten)),
      ALLOW_THIS_IN_INITIALIZER_LIST(
          channel_read_callback_(this, &PPB_Transport_Impl::OnRead)) {
}

PPB_Transport_Impl::~PPB_Transport_Impl() {
}

// static
PP_Resource PPB_Transport_Impl::Create(PluginInstance* instance,
                                       const char* name,
                                       const char* proto) {
  scoped_refptr<PPB_Transport_Impl> t(new PPB_Transport_Impl(instance));
  if (!t->Init(name, proto))
    return 0;
  return t->GetReference();
}

PPB_Transport_API* PPB_Transport_Impl::AsPPB_Transport_API() {
  return this;
}

bool PPB_Transport_Impl::Init(const char* name, const char* proto) {
  name_ = name;

  if (base::strcasecmp(proto, kUdpProtocolName) == 0) {
    use_tcp_ = false;
  } else if (base::strcasecmp(proto, kTcpProtocolName) == 0) {
    use_tcp_ = true;
  } else {
    LOG(WARNING) << "Unknown protocol: " << proto;
    return false;
  }

  p2p_transport_.reset(instance()->delegate()->CreateP2PTransport());
  return p2p_transport_.get() != NULL;
}

PP_Bool PPB_Transport_Impl::IsWritable() {
  if (!p2p_transport_.get())
    return PP_FALSE;

  return PP_FromBool(writable_);
}

int32_t PPB_Transport_Impl::Connect(PP_CompletionCallback callback) {
  if (!p2p_transport_.get())
    return PP_ERROR_FAILED;

  // Connect() has already been called.
  if (started_)
    return PP_ERROR_INPROGRESS;

  P2PTransport::Protocol protocol = use_tcp_ ?
      P2PTransport::PROTOCOL_TCP : P2PTransport::PROTOCOL_UDP;

  if (!p2p_transport_->Init(name_, protocol, "", this))
    return PP_ERROR_FAILED;

  started_ = true;

  PP_Resource resource_id = GetReferenceNoAddRef();
  CHECK(resource_id);
  connect_callback_ = new TrackedCompletionCallback(
      instance()->module()->GetCallbackTracker(), resource_id, callback);
  return PP_OK_COMPLETIONPENDING;
}

int32_t PPB_Transport_Impl::GetNextAddress(PP_Var* address,
                                           PP_CompletionCallback callback) {
  if (!p2p_transport_.get())
    return PP_ERROR_FAILED;

  if (next_address_callback_.get() && !next_address_callback_->completed())
    return PP_ERROR_INPROGRESS;

  if (!local_candidates_.empty()) {
    *address = StringVar::StringToPPVar(instance()->module()->pp_module(),
                                        local_candidates_.front());
    local_candidates_.pop_front();
    return PP_OK;
  }

  PP_Resource resource_id = GetReferenceNoAddRef();
  CHECK(resource_id);
  next_address_callback_ = new TrackedCompletionCallback(
      instance()->module()->GetCallbackTracker(), resource_id, callback);
  return PP_OK_COMPLETIONPENDING;
}

int32_t PPB_Transport_Impl::ReceiveRemoteAddress(PP_Var address) {
  if (!p2p_transport_.get())
    return PP_ERROR_FAILED;

  scoped_refptr<StringVar> address_str = StringVar::FromPPVar(address);
  if (!address_str)
    return PP_ERROR_BADARGUMENT;

  return p2p_transport_->AddRemoteCandidate(address_str->value()) ?
      PP_OK : PP_ERROR_FAILED;
}

int32_t PPB_Transport_Impl::Recv(void* data, uint32_t len,
                                 PP_CompletionCallback callback) {
  if (!p2p_transport_.get())
    return PP_ERROR_FAILED;

  if (recv_callback_.get() && !recv_callback_->completed())
    return PP_ERROR_INPROGRESS;

  net::Socket* channel = p2p_transport_->GetChannel();
  if (!channel)
    return PP_ERROR_FAILED;

  scoped_refptr<net::IOBuffer> buffer =
      new net::WrappedIOBuffer(static_cast<const char*>(data));
  int result = MapNetError(channel->Read(buffer, len, &channel_read_callback_));
  if (result == PP_OK_COMPLETIONPENDING) {
    PP_Resource resource_id = GetReferenceNoAddRef();
    CHECK(resource_id);
    recv_callback_ = new TrackedCompletionCallback(
        instance()->module()->GetCallbackTracker(), resource_id, callback);
  }

  return result;
}

int32_t PPB_Transport_Impl::Send(const void* data, uint32_t len,
                                 PP_CompletionCallback callback) {
  if (!p2p_transport_.get())
    return PP_ERROR_FAILED;

  if (send_callback_.get() && !send_callback_->completed())
    return PP_ERROR_INPROGRESS;

  net::Socket* channel = p2p_transport_->GetChannel();
  if (!channel)
    return PP_ERROR_FAILED;

  scoped_refptr<net::IOBuffer> buffer =
      new net::WrappedIOBuffer(static_cast<const char*>(data));
  int result = MapNetError(channel->Write(buffer, len,
                                          &channel_write_callback_));
  if (result == PP_OK_COMPLETIONPENDING) {
    PP_Resource resource_id = GetReferenceNoAddRef();
    CHECK(resource_id);
    send_callback_ = new TrackedCompletionCallback(
        instance()->module()->GetCallbackTracker(), resource_id, callback);
  }

  return result;
}

int32_t PPB_Transport_Impl::Close() {
  if (!p2p_transport_.get())
    return PP_ERROR_FAILED;

  p2p_transport_.reset();
  instance()->module()->GetCallbackTracker()->AbortAll();
  return PP_OK;
}

void PPB_Transport_Impl::OnCandidateReady(const std::string& address) {
  // Store the candidate first before calling the callback.
  local_candidates_.push_back(address);

  if (next_address_callback_.get() && !next_address_callback_->completed()) {
    scoped_refptr<TrackedCompletionCallback> callback;
    callback.swap(next_address_callback_);
    callback->Run(PP_OK);
  }
}

void PPB_Transport_Impl::OnStateChange(webkit_glue::P2PTransport::State state) {
  writable_ = (state | webkit_glue::P2PTransport::STATE_WRITABLE) != 0;
  if (writable_ && connect_callback_.get() && !connect_callback_->completed()) {
    scoped_refptr<TrackedCompletionCallback> callback;
    callback.swap(connect_callback_);
    callback->Run(PP_OK);
  }
}

void PPB_Transport_Impl::OnError(int error) {
  writable_ = false;
  if (connect_callback_.get() && !connect_callback_->completed()) {
    scoped_refptr<TrackedCompletionCallback> callback;
    callback.swap(connect_callback_);
    callback->Run(PP_ERROR_FAILED);
  }
}

void PPB_Transport_Impl::OnRead(int result) {
  DCHECK(recv_callback_.get() && !recv_callback_->completed());

  scoped_refptr<TrackedCompletionCallback> callback;
  callback.swap(recv_callback_);
  callback->Run(MapNetError(result));
}

void PPB_Transport_Impl::OnWritten(int result) {
  DCHECK(send_callback_.get() && !send_callback_->completed());

  scoped_refptr<TrackedCompletionCallback> callback;
  callback.swap(send_callback_);
  callback->Run(MapNetError(result));
}

}  // namespace ppapi
}  // namespace webkit
