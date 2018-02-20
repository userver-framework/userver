#pragma once

#include <engine/ev/thread_pool.hpp>

#include "component_base.hpp"
#include "component_config.hpp"
#include "component_context.hpp"

namespace components {

class ThreadPoolComponent : public ComponentBase {
 public:
  ThreadPoolComponent(const ComponentConfig& config,
                      const ComponentContext& context, const std::string& name);

  engine::ev::ThreadPool& Get();

 private:
  engine::ev::ThreadPool thread_pool_;
};

}  // namespace components
