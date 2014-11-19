// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_interceptor.h"

#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_protocol.h"
#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_usage_stats.h"
#include "components/data_reduction_proxy/core/common/data_reduction_proxy_headers.h"
#include "components/data_reduction_proxy/core/common/data_reduction_proxy_params.h"
#include "net/http/http_response_headers.h"
#include "net/url_request/url_request.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_http_job.h"
#include "net/url_request/url_request_job_factory.h"
#include "url/url_constants.h"

namespace data_reduction_proxy {

DataReductionProxyInterceptor::DataReductionProxyInterceptor(
    DataReductionProxyParams* params,
    DataReductionProxyUsageStats* stats)
    : params_(params),
      usage_stats_(stats) {
}

DataReductionProxyInterceptor::~DataReductionProxyInterceptor() {
}

net::URLRequestJob* DataReductionProxyInterceptor::MaybeInterceptRequest(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate) const {
  return nullptr;
}

net::URLRequestJob* DataReductionProxyInterceptor::MaybeInterceptResponse(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate) const {
  if (request->response_info().was_cached)
    return nullptr;
  DataReductionProxyBypassType bypass_type = BYPASS_EVENT_TYPE_MAX;
  bool should_retry = data_reduction_proxy::MaybeBypassProxyAndPrepareToRetry(
      params_, request, &bypass_type);
  if (usage_stats_ && bypass_type != BYPASS_EVENT_TYPE_MAX)
    usage_stats_->SetBypassType(bypass_type);
  if (!should_retry)
    return nullptr;
  // Returning non-NULL has the effect of restarting the request with the
  // supplied job.
  DCHECK(request->url().SchemeIs(url::kHttpScheme));
  return request->context()->job_factory()->MaybeCreateJobWithProtocolHandler(
      url::kHttpScheme, request, network_delegate);
}

}  // namespace data_reduction_proxy
