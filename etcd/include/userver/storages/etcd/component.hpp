#pragma once

/// @file userver/storages/redis/component.hpp
/// @brief @copybrief components::Redis

#include <memory>
#include <string>
#include <unordered_map>

#include <userver/components/component_fwd.hpp>
#include <userver/components/loggable_component_base.hpp>
#include <userver/dynamic_config/source.hpp>
#include <userver/utils/statistics/entry.hpp>

USERVER_NAMESPACE_BEGIN


/// Etcd client
namespace storages::etcd {
class Client;
}  // namespace storages::etcd

namespace components {

class Etcd : public LoggableComponentBase {
 public:
  Etcd(const ComponentConfig& config,
        const ComponentContext& component_context);
  ~Etcd() override;
  static constexpr std::string_view kName = "etcd";


 std::shared_ptr<storages::etcd::Client> GetClient() const;

private:
 std::unordered_map<std::string, std::shared_ptr<storages::etcd::Client>>
     clients_;

dynamic_config::Source config_;

};
}

USERVER_NAMESPACE_END

