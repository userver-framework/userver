#include <userver/engine/task/single_threaded_task_processors_pool.hpp>

USERVER_NAMESPACE_BEGIN

namespace utest {

[[deprecated(
    "Use engine::SingleThreadedTaskProcessorsPool::MakeForTests")]] engine::
    SingleThreadedTaskProcessorsPool
    MakeSingleThreadedTaskProcessorsPool(std::size_t worker_threads);
}  // namespace utest

USERVER_NAMESPACE_END
