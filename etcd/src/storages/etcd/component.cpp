#include <fmt/format.h>
#include <iterator>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <string>
#include <userver/storages/etcd/component.hpp>

//#include <stdexcept>
//#include <vector>

#include <userver/components/component.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/dynamic_config/storage/component.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/logging/log.hpp>
#include <userver/storages/etcd/client.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/storages/secdist/exceptions.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/statistics/aggregated_values.hpp>
#include <userver/utils/statistics/metadata.hpp>
#include <userver/utils/statistics/percentile_format_json.hpp>
#include <userver/yaml_config/merge_schemas.hpp>
#include <vector>
#include "client_impl.hpp"
#include "userver/storages/etcd/client_fwd.hpp"
#include "userver/utils/assert.hpp"
#include "userver/yaml_config/yaml_config.hpp"

#include <etcd/api/etcdserverpb/rpc_client.usrv.pb.hpp>
#include <userver/components/component.hpp>
#include <userver/ugrpc/client/client_factory_component.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

Etcd::Etcd(const ComponentConfig& config,
           const ComponentContext& component_context)
    : LoggableComponentBase(config, component_context),
      grpc_client_factory_(
          component_context
              .FindComponent<userver::ugrpc::client::ClientFactoryComponent>()
              .GetFactory()),
      config_(component_context.FindComponent<DynamicConfig>().GetSource()) {
  Connect(config);
}

storages::etcd::ClientPtr Etcd::GetClient(const std::string& endpoint) const {
  if (endpoint.empty()) {
    if (clients_.empty())
      throw std::runtime_error("Etcd endpoints not provided");
    else
      return clients_.begin()->second;
  }

  const auto it = clients_.find(endpoint);
  if (it == clients_.cend())
    throw std::runtime_error(
        fmt::format("Etcd client not found at {}", endpoint));
  return it->second;
}

Etcd::~Etcd() {
  try {
    // statistics_holder_.Unregister();
    // subscribe_statistics_holder_.Unregister();
    // config_subscription_.Unsubscribe();
  } catch (std::exception const& e) {
    LOG_ERROR() << "exception while destroying Etcd component: " << e;
  } catch (...) {
    LOG_ERROR() << "non-standard exception while destroying Etcd component";
  }
}

void Etcd::Connect(const ComponentConfig& config) {
  auto endpoints = config["endpoints"].As<std::vector<std::string>>();
  for (const auto& endpoint : endpoints) {
    auto grpc_client = std::make_unique<etcdserverpb::KVClient>(
        grpc_client_factory_.MakeClient<etcdserverpb::KVClient>(endpoint));
    clients_.emplace(endpoint, std::make_shared<storages::etcd::ClientImpl>(
                                   std::move(grpc_client)));
  }
}

}  // namespace components

USERVER_NAMESPACE_END
