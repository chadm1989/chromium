// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_SIGNIN_TOKEN_HANDLE_FETCHER_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_SIGNIN_TOKEN_HANDLE_FETCHER_H_

#include <string>

#include "base/basictypes.h"
#include "base/callback.h"
#include "base/containers/scoped_ptr_hash_map.h"
#include "base/memory/weak_ptr.h"
#include "components/user_manager/user_id.h"
#include "google_apis/gaia/gaia_oauth_client.h"
#include "google_apis/gaia/oauth2_token_service.h"

class TokenHandleUtil;
class Profile;

// This class is resposible for obtaining new token handle for user.
// It can be used in two ways. When user have just used GAIA signin there is
// an OAuth2 token available. If there is profile already loaded, then
// minting additional access token might be required.

class TokenHandleFetcher : public gaia::GaiaOAuthClient::Delegate,
                           public OAuth2TokenService::Consumer,
                           public OAuth2TokenService::Observer {
 public:
  TokenHandleFetcher(TokenHandleUtil* util,
                     const user_manager::UserID& user_id);
  ~TokenHandleFetcher() override;

  typedef base::Callback<void(const user_manager::UserID&, bool success)>
      TokenFetchingCallback;

  // Get token handle for user who have just signed in via GAIA. This
  // request will be performed using signin profile.
  void FillForNewUser(const std::string& access_token,
                      const TokenFetchingCallback& callback);

  // Get token handle for existing user.
  void BackfillToken(Profile* profile, const TokenFetchingCallback& callback);

 private:
  // OAuth2TokenService::Observer override:
  void OnRefreshTokenAvailable(const std::string& account_id) override;

  // OAuth2TokenService::Consumer overrides:
  void OnGetTokenSuccess(const OAuth2TokenService::Request* request,
                         const std::string& access_token,
                         const base::Time& expiration_time) override;
  void OnGetTokenFailure(const OAuth2TokenService::Request* request,
                         const GoogleServiceAuthError& error) override;

  // GaiaOAuthClient::Delegate overrides:
  void OnOAuthError() override;
  void OnNetworkError(int response_code) override;
  void OnGetTokenInfoResponse(
      scoped_ptr<base::DictionaryValue> token_info) override;

  void RequestAccessToken(const std::string& account_id);
  void FillForAccessToken(const std::string& access_token);

  TokenHandleUtil* token_handle_util_;
  user_manager::UserID user_id_;
  OAuth2TokenService* token_service_;

  bool waiting_for_refresh_token_;
  std::string account_without_token_;
  Profile* profile_;
  TokenFetchingCallback callback_;
  scoped_ptr<gaia::GaiaOAuthClient> gaia_client_;
  scoped_ptr<OAuth2TokenService::Request> oauth2_access_token_request_;

  DISALLOW_COPY_AND_ASSIGN(TokenHandleFetcher);
};

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_SIGNIN_TOKEN_HANDLE_FETCHER_H_
