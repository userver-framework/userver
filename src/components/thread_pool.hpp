#pragma once

#include <engine/ev/thread_pool.hpp>

#include "component_base.hpp"
#include "component_config.hpp"
#include "component_context.hpp"

namespace components {

class ThreadPool : public ComponentBase {
 public:
  ThreadPool(const ComponentConfig& config, const ComponentContext& context);

  engine::ev::ThreadPool& Get();

 private:
  engine::ev::ThreadPool thread_pool_;
};

}  // namespace components
