#include <yandex/taxi/userver/components/thread_pool.hpp>

#include <yandex/taxi/userver/json_config/value.hpp>

namespace components {

ThreadPool::ThreadPool(const ComponentConfig& config, const ComponentContext&)
    : thread_pool_(config.ParseUint64("threads"),
                   config.ParseString("thread_name")) {}

engine::ev::ThreadPool& ThreadPool::Get() { return thread_pool_; }

}  // namespace components
