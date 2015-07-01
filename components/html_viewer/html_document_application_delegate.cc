// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/html_viewer/html_document_application_delegate.h"

#include "base/bind.h"
#include "base/command_line.h"
#include "components/html_viewer/global_state.h"
#include "components/html_viewer/html_document.h"
#include "components/html_viewer/html_document_oopif.h"
#include "components/html_viewer/html_viewer_switches.h"
#include "mojo/application/public/cpp/application_connection.h"
#include "mojo/application/public/cpp/application_delegate.h"
#include "mojo/application/public/cpp/connect.h"

namespace html_viewer {

namespace {

bool EnableOOPIFs() {
  return base::CommandLine::ForCurrentProcess()->HasSwitch(switches::kOOPIF);
}

}  // namespace

HTMLDocumentApplicationDelegate::HTMLDocumentApplicationDelegate(
    mojo::InterfaceRequest<mojo::Application> request,
    mojo::URLResponsePtr response,
    GlobalState* global_state,
    scoped_ptr<mojo::AppRefCount> parent_app_refcount)
    : app_(this,
           request.Pass(),
           base::Bind(&HTMLDocumentApplicationDelegate::OnTerminate,
                      base::Unretained(this))),
      parent_app_refcount_(parent_app_refcount.Pass()),
      url_(response->url),
      initial_response_(response.Pass()),
      global_state_(global_state) {
}

HTMLDocumentApplicationDelegate::~HTMLDocumentApplicationDelegate() {
  // Deleting the documents is going to trigger a callback to
  // OnHTMLDocumentDeleted() and remove from |documents_|. Copy the set so we
  // don't have to worry about the set being modified out from under us.
  std::set<HTMLDocument*> documents(documents_);
  for (HTMLDocument* doc : documents)
    doc->Destroy();
  DCHECK(documents_.empty());

  std::set<HTMLDocumentOOPIF*> documents2(documents2_);
  for (HTMLDocumentOOPIF* doc : documents2)
    doc->Destroy();
  DCHECK(documents2_.empty());
}

// Callback from the quit closure. We key off this rather than
// ApplicationDelegate::Quit() as we don't want to shut down the messageloop
// when we quit (the messageloop is shared among multiple
// HTMLDocumentApplicationDelegates).
void HTMLDocumentApplicationDelegate::OnTerminate() {
  delete this;
}

// ApplicationDelegate;
void HTMLDocumentApplicationDelegate::Initialize(mojo::ApplicationImpl* app) {
  mojo::URLRequestPtr request(mojo::URLRequest::New());
  request->url = mojo::String::From("mojo:network_service");
  mojo::ApplicationConnection* connection =
      app_.ConnectToApplication(request.Pass());
  connection->ConnectToService(&network_service_);
  connection->ConnectToService(&url_loader_factory_);
}

bool HTMLDocumentApplicationDelegate::ConfigureIncomingConnection(
    mojo::ApplicationConnection* connection) {
  if (initial_response_) {
    OnResponseReceived(mojo::URLLoaderPtr(), connection,
                       initial_response_.Pass());
  } else {
    mojo::URLLoaderPtr loader;
    url_loader_factory_->CreateURLLoader(GetProxy(&loader));
    mojo::URLRequestPtr request(mojo::URLRequest::New());
    request->url = url_;
    request->auto_follow_redirects = true;

    // |loader| will be passed to the OnResponseReceived method through a
    // callback. Because order of evaluation is undefined, a reference to the
    // raw pointer is needed.
    mojo::URLLoader* raw_loader = loader.get();
    raw_loader->Start(
        request.Pass(),
        base::Bind(&HTMLDocumentApplicationDelegate::OnResponseReceived,
                   base::Unretained(this), base::Passed(&loader), connection));
  }
  return true;
}

void HTMLDocumentApplicationDelegate::OnHTMLDocumentDeleted(
    HTMLDocument* document) {
  DCHECK(documents_.count(document) > 0);
  documents_.erase(document);
}

void HTMLDocumentApplicationDelegate::OnHTMLDocumentDeleted2(
    HTMLDocumentOOPIF* document) {
  DCHECK(documents2_.count(document) > 0);
  documents2_.erase(document);
}

void HTMLDocumentApplicationDelegate::OnResponseReceived(
    mojo::URLLoaderPtr loader,
    mojo::ApplicationConnection* connection,
    mojo::URLResponsePtr response) {
  // HTMLDocument is destroyed when the hosting view is destroyed, or
  // explicitly from our destructor.
  if (EnableOOPIFs()) {
    HTMLDocumentOOPIF* document = new HTMLDocumentOOPIF(
        &app_, connection, response.Pass(), global_state_,
        base::Bind(&HTMLDocumentApplicationDelegate::OnHTMLDocumentDeleted2,
                   base::Unretained(this)));
    documents2_.insert(document);
  } else {
    HTMLDocument::CreateParams params(
        &app_, connection, response.Pass(), global_state_,
        base::Bind(&HTMLDocumentApplicationDelegate::OnHTMLDocumentDeleted,
                   base::Unretained(this)));
    HTMLDocument* document = new HTMLDocument(&params);
    documents_.insert(document);
  }
}

}  // namespace html_viewer
