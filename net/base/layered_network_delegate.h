// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_BASE_LAYERED_NETWORK_DELEGATE_H_
#define NET_BASE_LAYERED_NETWORK_DELEGATE_H_

#include <stdint.h>

#include "base/memory/scoped_ptr.h"
#include "base/strings/string16.h"
#include "net/base/completion_callback.h"
#include "net/base/network_delegate.h"
#include "net/cookies/canonical_cookie.h"

class GURL;

namespace base {
class FilePath;
}

namespace net {

class CookieOptions;
class HttpRequestHeaders;
class HttpResponseHeaders;
class ProxyInfo;
class ProxyServer;
class ProxyService;
class URLRequest;

// WrappingNetworkDelegate takes a |network_delegate| and extends it. When
// On*() is called, the On*Internal() method of this is first called and then
// the On*() of |network_delegate| is called. On*Internal() methods have no
// return values, and cannot prevent calling into the nested network delegate.
class NET_EXPORT LayeredNetworkDelegate : public NetworkDelegate {
 public:
  explicit LayeredNetworkDelegate(
      scoped_ptr<NetworkDelegate> nested_network_delegate);
  ~LayeredNetworkDelegate() override;

  // NetworkDelegate implementation:
  int OnBeforeURLRequest(URLRequest* request,
                         const CompletionCallback& callback,
                         GURL* new_url) final;
  void OnResolveProxy(const GURL& url,
                      int load_flags,
                      const ProxyService& proxy_service,
                      ProxyInfo* result) final;
  void OnProxyFallback(const ProxyServer& bad_proxy, int net_error) final;
  int OnBeforeSendHeaders(URLRequest* request,
                          const CompletionCallback& callback,
                          HttpRequestHeaders* headers) final;
  void OnBeforeSendProxyHeaders(URLRequest* request,
                                const ProxyInfo& proxy_info,
                                HttpRequestHeaders* headers) final;
  void OnSendHeaders(URLRequest* request,
                     const HttpRequestHeaders& headers) final;
  int OnHeadersReceived(
      URLRequest* request,
      const CompletionCallback& callback,
      const HttpResponseHeaders* original_response_headers,
      scoped_refptr<HttpResponseHeaders>* override_response_headers,
      GURL* allowed_unsafe_redirect_url) final;
  void OnBeforeRedirect(URLRequest* request, const GURL& new_location) final;
  void OnResponseStarted(URLRequest* request) final;
  void OnNetworkBytesReceived(const URLRequest& request,
                              int64_t bytes_received) final;
  void OnCompleted(URLRequest* request, bool started) final;
  void OnURLRequestDestroyed(URLRequest* request) final;
  void OnPACScriptError(int line_number, const base::string16& error) final;
  AuthRequiredResponse OnAuthRequired(URLRequest* request,
                                      const AuthChallengeInfo& auth_info,
                                      const AuthCallback& callback,
                                      AuthCredentials* credentials) final;
  bool OnCanGetCookies(const URLRequest& request,
                       const CookieList& cookie_list) final;
  bool OnCanSetCookie(const URLRequest& request,
                      const std::string& cookie_line,
                      CookieOptions* options) final;
  bool OnCanAccessFile(const URLRequest& request,
                       const base::FilePath& path) const final;
  bool OnCanEnablePrivacyMode(const GURL& url,
                              const GURL& first_party_for_cookies) const final;
  bool OnFirstPartyOnlyCookieExperimentEnabled() const final;
  bool OnCancelURLRequestWithPolicyViolatingReferrerHeader(
      const URLRequest& request,
      const GURL& target_url,
      const GURL& referrer_url) const final;

 protected:
  virtual void OnBeforeURLRequestInternal(URLRequest* request,
                                          const CompletionCallback& callback,
                                          GURL* new_url);

  virtual void OnResolveProxyInternal(const GURL& url,
                                      int load_flags,
                                      const ProxyService& proxy_service,
                                      ProxyInfo* result);

  virtual void OnProxyFallbackInternal(const ProxyServer& bad_proxy,
                                       int net_error);

  virtual void OnBeforeSendHeadersInternal(URLRequest* request,
                                           const CompletionCallback& callback,
                                           HttpRequestHeaders* headers);

  virtual void OnBeforeSendProxyHeadersInternal(URLRequest* request,
                                                const ProxyInfo& proxy_info,
                                                HttpRequestHeaders* headers);

  virtual void OnSendHeadersInternal(URLRequest* request,
                                     const HttpRequestHeaders& headers);

  virtual void OnHeadersReceivedInternal(
      URLRequest* request,
      const CompletionCallback& callback,
      const HttpResponseHeaders* original_response_headers,
      scoped_refptr<HttpResponseHeaders>* override_response_headers,
      GURL* allowed_unsafe_redirect_url);

  virtual void OnBeforeRedirectInternal(URLRequest* request,
                                        const GURL& new_location);

  virtual void OnResponseStartedInternal(URLRequest* request);

  virtual void OnNetworkBytesReceivedInternal(const URLRequest& request,
                                              int64_t bytes_received);

  virtual void OnCompletedInternal(URLRequest* request, bool started);

  virtual void OnURLRequestDestroyedInternal(URLRequest* request);

  virtual void OnPACScriptErrorInternal(int line_number,
                                        const base::string16& error);

  virtual void OnCanGetCookiesInternal(const URLRequest& request,
                                       const CookieList& cookie_list);

  virtual void OnCanSetCookieInternal(const URLRequest& request,
                                      const std::string& cookie_line,
                                      CookieOptions* options);

  virtual void OnAuthRequiredInternal(URLRequest* request,
                                      const AuthChallengeInfo& auth_info,
                                      const AuthCallback& callback,
                                      AuthCredentials* credentials);

  virtual void OnCanAccessFileInternal(const URLRequest& request,
                                       const base::FilePath& path) const;

  virtual void OnCanEnablePrivacyModeInternal(
      const GURL& url,
      const GURL& first_party_for_cookies) const;

  virtual void OnFirstPartyOnlyCookieExperimentEnabledInternal() const;

  virtual void OnCancelURLRequestWithPolicyViolatingReferrerHeaderInternal(
      const URLRequest& request,
      const GURL& target_url,
      const GURL& referrer_url) const;

 private:
  scoped_ptr<NetworkDelegate> nested_network_delegate_;
};

}  // namespace net

#endif  // NET_BASE_LAYERED_NETWORK_DELEGATE_H_
