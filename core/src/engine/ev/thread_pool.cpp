#include "thread_pool.hpp"

#include "thread.hpp"
#include "thread_control.hpp"

#include <utils/assert.hpp>

namespace engine {
namespace ev {

ThreadPool::ThreadPool(ThreadPoolConfig config)
    : ThreadPool(std::move(config), false) {}

ThreadPool::ThreadPool(ThreadPoolConfig config, UseDefaultEvLoop)
    : ThreadPool(std::move(config), true) {}

ThreadPool::ThreadPool(ThreadPoolConfig config, bool use_ev_default_loop)
    : use_ev_default_loop_(use_ev_default_loop) {
  threads_.reserve(config.threads);
  for (size_t i = 0; i < config.threads; i++) {
    threads_.emplace_back(
        use_ev_default_loop_ && !i
            ? std::make_unique<Thread>(
                  config.thread_name + '_' + std::to_string(i),
                  Thread::kUseDefaultEvLoop)
            : std::make_unique<Thread>(config.thread_name + '_' +
                                       std::to_string(i)));
  }
  info_ = std::make_unique<ThreadPoolInfo>(*this);
}

ThreadPool::ThreadPoolInfo::ThreadPoolInfo(ThreadPool& thread_pool)
    : counter_(0) {
  thread_controls_.reserve(thread_pool.size());
  for (const auto& thread : thread_pool.threads_)
    thread_controls_.emplace_back(*thread);
}

ThreadControl& ThreadPool::ThreadPoolInfo::NextThread() {
  UASSERT(!thread_controls_.empty());
  // just ignore counter_ overflow
  return thread_controls_[counter_++ % thread_controls_.size()];
}

std::vector<ThreadControl*> ThreadPool::ThreadPoolInfo::NextThreads(
    size_t count) {
  std::vector<ThreadControl*> res;
  if (!count) return res;
  UASSERT(!thread_controls_.empty());
  res.reserve(count);
  size_t start = counter_.fetch_add(count);  // just ignore counter_ overflow
  for (size_t i = 0; i < count; i++)
    res.emplace_back(&thread_controls_[(start + i) % thread_controls_.size()]);
  return res;
}

ThreadControl& ThreadPool::ThreadPoolInfo::GetThread(size_t idx) {
  return thread_controls_.at(idx);
}

ThreadControl& ThreadPool::GetEvDefaultLoopThread() {
  UASSERT(use_ev_default_loop_);
  if (!use_ev_default_loop_)
    throw std::runtime_error("no ev_default_loop in current thread_pool");
  return info_->GetThread(0);
}

}  // namespace ev
}  // namespace engine
