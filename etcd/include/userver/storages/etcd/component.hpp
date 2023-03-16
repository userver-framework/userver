#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include <userver/components/component_fwd.hpp>
#include <userver/components/loggable_component_base.hpp>
#include <userver/dynamic_config/source.hpp>
#include <userver/utils/statistics/entry.hpp>

#include "fwd.hpp"

USERVER_NAMESPACE_BEGIN

namespace components {

class Etcd : public LoggableComponentBase {
 public:
  Etcd(const ComponentConfig& config,
       const ComponentContext& component_context);
  ~Etcd() override;
  static constexpr std::string_view kName = "etcd";

  storages::etcd::ClientPtr GetClient(
      const std::string& endpoint = std::string()) const;

 private:
  void Connect(const ComponentConfig& config, const ComponentContext& context);

  std::unordered_map<std::string, storages::etcd::ClientPtr> clients_;
  userver::ugrpc::client::ClientFactory& grpc_client_factory_;
  std::unordered_map<std::string, etcdserverpb::KVClientUPtr> grpc_clients_;

  dynamic_config::Source config_;
};

}  // namespace components

USERVER_NAMESPACE_END