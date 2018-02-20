#include "thread_pool_component.hpp"

#include <json_config/value.hpp>

namespace components {

ThreadPoolComponent::ThreadPoolComponent(const ComponentConfig& config,
                                         const ComponentContext&,
                                         const std::string& name)
    : thread_pool_(config.ParseUint64("threads"),
                   config.ParseString("thread_name", name)) {}

engine::ev::ThreadPool& ThreadPoolComponent::Get() { return thread_pool_; }

}  // namespace components
