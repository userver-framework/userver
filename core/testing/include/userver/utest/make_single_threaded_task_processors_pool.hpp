#include <userver/engine/task/single_threaded_task_processors_pool.hpp>

namespace utest {

engine::SingleThreadedTaskProcessorsPool MakeSingleThreadedTaskProcessorsPool(
    std::size_t worker_threads);
}  // namespace utest
