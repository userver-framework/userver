#pragma once

#include <atomic>
#include <memory>
#include <string>
#include <vector>

#include <engine/ev/thread_control.hpp>
#include <engine/ev/thread_pool_config.hpp>

namespace engine {
namespace ev {

class ThreadPool final {
 public:
  ThreadPool(ThreadPoolConfig config);

  inline size_t size() const { return threads_.size(); }

  ThreadControl& NextThread() { return info_->NextThread(); }

  std::vector<ThreadControl*> NextThreads(size_t count) {
    return info_->NextThreads(count);
  }

 private:
  class ThreadPoolInfo final {
   public:
    explicit ThreadPoolInfo(ThreadPool& thread_pool);

    ThreadControl& NextThread();
    std::vector<ThreadControl*> NextThreads(size_t count);

   private:
    std::vector<ThreadControl> thread_controls_;
    std::atomic<size_t> counter_;
  };

  std::vector<std::unique_ptr<Thread>> threads_;
  std::unique_ptr<ThreadPoolInfo> info_;
};

}  // namespace ev
}  // namespace engine
