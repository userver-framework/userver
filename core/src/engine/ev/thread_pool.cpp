#include "thread_pool.hpp"

#include <cassert>

#include "thread.hpp"
#include "thread_control.hpp"

namespace engine {
namespace ev {

ThreadPool::ThreadPool(ThreadPoolConfig config) {
  threads_.reserve(config.threads);
  for (size_t i = 0; i < config.threads; i++)
    threads_.emplace_back(
        std::make_unique<Thread>(config.thread_name + '_' + std::to_string(i)));
  info_ = std::make_unique<ThreadPoolInfo>(*this);
}

ThreadPool::ThreadPoolInfo::ThreadPoolInfo(ThreadPool& thread_pool)
    : counter_(0) {
  thread_controls_.reserve(thread_pool.size());
  for (const auto& thread : thread_pool.threads_)
    thread_controls_.emplace_back(*thread);
}

ThreadControl& ThreadPool::ThreadPoolInfo::NextThread() {
  assert(!thread_controls_.empty());
  // just ignore counter_ overflow
  return thread_controls_[counter_++ % thread_controls_.size()];
}

std::vector<ThreadControl*> ThreadPool::ThreadPoolInfo::NextThreads(
    size_t count) {
  std::vector<ThreadControl*> res;
  if (!count) return res;
  assert(!thread_controls_.empty());
  res.reserve(count);
  size_t start = counter_.fetch_add(count);  // just ignore counter_ overflow
  for (size_t i = 0; i < count; i++)
    res.emplace_back(&thread_controls_[(start + i) % thread_controls_.size()]);
  return res;
}

}  // namespace ev
}  // namespace engine
