#pragma once

#include <memory>
#include <vector>

#include <userver/engine/task/task_processor_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

struct TaskProcessorConfig;

class SingleThreadedTaskProcessorsPool final {
 public:
  // Do NOT use directly! Use components::SingleThreadedTaskProcessors or for
  // tests and benchmarks use SingleThreadedTaskProcessorsPool::MakeForTests()
  explicit SingleThreadedTaskProcessorsPool(
      const engine::TaskProcessorConfig& config_base);
  ~SingleThreadedTaskProcessorsPool();

  size_t GetSize() const noexcept { return processors_.size(); }
  engine::TaskProcessor& At(size_t idx) { return *processors_.at(idx); }

  // For tests and benchmarks only
  static SingleThreadedTaskProcessorsPool MakeForTests(
      std::size_t worker_threads);

 private:
  std::vector<std::unique_ptr<engine::TaskProcessor>> processors_;
};

}  // namespace engine

USERVER_NAMESPACE_END
