#pragma once

#include <atomic>
#include <memory>
#include <string>
#include <vector>

#include "thread_control.hpp"

namespace engine {
namespace ev {

class ThreadPool {
 public:
  ThreadPool(size_t thread_count, const std::string& thread_name);
  ~ThreadPool();

  inline size_t size() const { return threads_.size(); }

  ThreadControl& NextThread() { return info_->NextThread(); }

  std::vector<ThreadControl*> NextThreads(size_t count) {
    return info_->NextThreads(count);
  }

 private:
  class ThreadPoolInfo {
   public:
    explicit ThreadPoolInfo(ThreadPool& thread_pool);
    ~ThreadPoolInfo();

    ThreadControl& NextThread();
    std::vector<ThreadControl*> NextThreads(size_t count);

   private:
    std::vector<ThreadControl> thread_controls_;
    std::atomic<size_t> counter_{0};
  };

  std::vector<std::unique_ptr<Thread>> threads_;
  std::unique_ptr<ThreadPoolInfo> info_;
};

}  // namespace ev
}  // namespace engine
