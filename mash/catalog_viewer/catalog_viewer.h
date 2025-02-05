// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MASH_CATALOG_VIEWER_CATALOG_VIEWER_H_
#define MASH_CATALOG_VIEWER_CATALOG_VIEWER_H_

#include <map>
#include <memory>

#include "base/callback.h"
#include "base/macros.h"
#include "mash/public/interfaces/launchable.mojom.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "services/shell/public/cpp/service.h"
#include "services/tracing/public/cpp/tracing_impl.h"

namespace views {
class AuraInit;
class Widget;
class WindowManagerConnection;
}

namespace mash {
namespace catalog_viewer {

class CatalogViewer : public shell::Service,
                      public mojom::Launchable,
                      public shell::InterfaceFactory<mojom::Launchable> {
 public:
  CatalogViewer();
  ~CatalogViewer() override;

  void RemoveWindow(views::Widget* window);

 private:
  // shell::Service:
  void OnStart(shell::Connector* connector,
               const shell::Identity& identity,
               uint32_t id) override;
  bool OnConnect(shell::Connection* connection) override;

  // mojom::Launchable:
  void Launch(uint32_t what, mojom::LaunchMode how) override;

  // shell::InterfaceFactory<mojom::Launchable>:
  void Create(shell::Connection* connection,
              mojom::LaunchableRequest request) override;


  shell::Connector* connector_ = nullptr;
  mojo::BindingSet<mojom::Launchable> bindings_;
  std::vector<views::Widget*> windows_;

  mojo::TracingImpl tracing_;
  std::unique_ptr<views::AuraInit> aura_init_;
  std::unique_ptr<views::WindowManagerConnection> window_manager_connection_;

  DISALLOW_COPY_AND_ASSIGN(CatalogViewer);
};

}  // namespace catalog_viewer
}  // namespace mash

#endif  // MASH_CATALOG_VIEWER_CATALOG_VIEWER_H_
