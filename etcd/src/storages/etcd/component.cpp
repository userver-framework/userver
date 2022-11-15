#include <iterator>
#include <memory>
#include <numeric>
#include <string>
#include <userver/storages/etcd/component.hpp>

//#include <stdexcept>
//#include <vector>

#include <userver/components/component.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/dynamic_config/storage/component.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/logging/log.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/storages/secdist/exceptions.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/statistics/aggregated_values.hpp>
#include <userver/utils/statistics/metadata.hpp>
#include <userver/utils/statistics/percentile_format_json.hpp>
#include <userver/yaml_config/merge_schemas.hpp>
#include <userver/storages/etcd/client.hpp>
#include <vector>
#include "client_impl.hpp"
#include "userver/utils/assert.hpp"
#include "userver/yaml_config/yaml_config.hpp"

USERVER_NAMESPACE_BEGIN

namespace components {

Etcd::Etcd(const ComponentConfig& config,
             const ComponentContext& component_context)
    : LoggableComponentBase(config, component_context),
      config_(component_context.FindComponent<DynamicConfig>().GetSource())
{
  [[maybe_unused]] auto endpoints = config["endpoints"].As<std::vector<std::string>>();
  LOG_INFO() << "Etcd Endpoints: ";
  for (const auto& endpoint: endpoints) {
    LOG_INFO() << endpoint;
  }
  // TODO: создавтаь клиента на основе полученного endpoint-а.
  client_ = std::make_shared<storages::etcd::ClientImpl>(endpoints.front());
}

std::shared_ptr<storages::etcd::Client> Etcd::GetClient() const
{
  return client_;
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


}  // namespace components

USERVER_NAMESPACE_END
