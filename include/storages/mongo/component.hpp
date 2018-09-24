#pragma once

#include <memory>

#include <components/component_base.hpp>
#include <components/component_config.hpp>
#include <components/component_context.hpp>

#include "pool.hpp"

namespace engine {
class TaskProcessor;
}  // namespace engine

namespace components {

class Mongo : public ComponentBase {
 public:
  static constexpr int kDefaultConnTimeoutMs = 5 * 1000;
  static constexpr int kDefaultSoTimeoutMs = 30 * 1000;
  static constexpr size_t kDefaultMinPoolSize = 32;
  static constexpr size_t kDefaultMaxPoolSize = 256;
  static constexpr size_t kDefaultThreadsNum = 2;

  Mongo(const ComponentConfig&, const ComponentContext&);
  ~Mongo();

  storages::mongo::PoolPtr GetPool() const;

 private:
  std::unique_ptr<engine::TaskProcessor> task_processor_;
  storages::mongo::PoolPtr pool_;
};

}  // namespace components
