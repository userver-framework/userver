#include <userver/utest/make_single_threaded_task_processors_pool.hpp>

USERVER_NAMESPACE_BEGIN

namespace utest {

engine::SingleThreadedTaskProcessorsPool MakeSingleThreadedTaskProcessorsPool(
    std::size_t worker_threads) {
  return engine::SingleThreadedTaskProcessorsPool::MakeForTests(worker_threads);
}

}  // namespace utest

USERVER_NAMESPACE_END
