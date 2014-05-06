// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_POLICY_CORE_COMMON_CLOUD_USER_INFO_FETCHER_H_
#define COMPONENTS_POLICY_CORE_COMMON_CLOUD_USER_INFO_FETCHER_H_

#include <string>
#include "base/memory/scoped_ptr.h"
#include "components/policy/policy_export.h"
#include "google_apis/gaia/gaia_oauth_client.h"
#include "net/url_request/url_fetcher_delegate.h"

class GoogleServiceAuthError;

namespace base {
class DictionaryValue;
}

namespace net {
class URLFetcher;
class URLRequestContextGetter;
}

namespace policy {

// Class that makes a UserInfo request, parses the response, and notifies
// a provided Delegate when the request is complete.
class POLICY_EXPORT UserInfoFetcher {
 public:
  class POLICY_EXPORT Delegate {
   public:
    // Invoked when the UserInfo request has succeeded, passing the parsed
    // response in |response|. Delegate may free the UserInfoFetcher in this
    // callback.
    virtual void OnGetUserInfoSuccess(
        const base::DictionaryValue* response) = 0;

    // Invoked when the UserInfo request has failed, passing the associated
    // error in |error|. Delegate may free the UserInfoFetcher in this
    // callback.
    virtual void OnGetUserInfoFailure(const GoogleServiceAuthError& error) = 0;
  };

  // Create a new UserInfoFetcher. |context| can be NULL for unit tests.
  UserInfoFetcher(Delegate* delegate, net::URLRequestContextGetter* context);
  virtual ~UserInfoFetcher();

  // Starts the UserInfo request, using the passed OAuth2 |access_token|.
  void Start(const std::string& access_token);

 private:
  class GaiaDelegate : public gaia::GaiaOAuthClient::Delegate {
   public:
    explicit GaiaDelegate(UserInfoFetcher::Delegate* delegate);

   private:
    // gaia::GaiaOAuthClient::Delegate implementation.
    virtual void OnGetUserInfoResponse(
        scoped_ptr<base::DictionaryValue> user_info) OVERRIDE;
    virtual void OnOAuthError() OVERRIDE;
    virtual void OnNetworkError(int response_code) OVERRIDE;

    UserInfoFetcher::Delegate* delegate_;

    DISALLOW_COPY_AND_ASSIGN(GaiaDelegate);
  };

  GaiaDelegate delegate_;
  gaia::GaiaOAuthClient gaia_client_;

  DISALLOW_COPY_AND_ASSIGN(UserInfoFetcher);
};

}  // namespace policy

#endif  // COMPONENTS_POLICY_CORE_COMMON_CLOUD_USER_INFO_FETCHER_H_
