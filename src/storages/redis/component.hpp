#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include <components/component_base.hpp>
#include <components/component_config.hpp>
#include <components/component_context.hpp>

namespace storages {
namespace redis {
class Sentinel;
}  // namespace redis
}  // namespace storages

namespace components {

class Redis : public ComponentBase {
 public:
  Redis(const ComponentConfig& config,
        const ComponentContext& component_context);

  static constexpr const char* const kName = "redis";

  std::shared_ptr<storages::redis::Sentinel> Client(const std::string& name) {
    return clients_.at(name);
  }

 private:
  std::unordered_map<std::string, std::shared_ptr<storages::redis::Sentinel>>
      clients_;
};

}  // namespace components
