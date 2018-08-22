#pragma once

#include <yandex/taxi/userver/components/component_base.hpp>
#include <yandex/taxi/userver/components/component_config.hpp>
#include <yandex/taxi/userver/components/component_context.hpp>

#include "pool.hpp"

namespace components {

class Mongo : public ComponentBase {
 public:
  static constexpr int kDefaultSoTimeoutMs = 30 * 1000;
  static constexpr size_t kDefaultMinPoolSize = 32;
  static constexpr size_t kDefaultMaxPoolSize = 256;

  Mongo(const ComponentConfig&, const ComponentContext&);

  storages::mongo::PoolPtr GetPool() const;

 private:
  storages::mongo::PoolPtr pool_;
};

}  // namespace components
