#include <userver/utest/make_single_threaded_task_processors_pool.hpp>

#include <engine/task/task_processor_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace utest {

engine::SingleThreadedTaskProcessorsPool MakeSingleThreadedTaskProcessorsPool(
    std::size_t worker_threads) {
  engine::TaskProcessorConfig config;
  config.name = "test";
  config.worker_threads = worker_threads;

  return engine::SingleThreadedTaskProcessorsPool{config};
}

}  // namespace utest

USERVER_NAMESPACE_END
