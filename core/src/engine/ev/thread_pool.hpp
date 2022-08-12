#pragma once

#include <atomic>
#include <vector>

#include <engine/ev/thread_control.hpp>
#include <engine/ev/thread_pool_config.hpp>
#include <userver/utils/fixed_array.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::ev {

class Thread;

class ThreadPool final {
 public:
  struct UseDefaultEvLoop {};
  static constexpr UseDefaultEvLoop kUseDefaultEvLoop{};

  explicit ThreadPool(ThreadPoolConfig config);
  ThreadPool(ThreadPoolConfig config, UseDefaultEvLoop);

  ~ThreadPool();

  std::size_t GetSize() const;

  ThreadControl& NextThread();

  std::vector<ThreadControl*> NextThreads(std::size_t count);

  ThreadControl& GetEvDefaultLoopThread();

 private:
  ThreadPool(ThreadPoolConfig config, bool use_ev_default_loop);

  bool use_ev_default_loop_;
  utils::FixedArray<Thread> threads_;
  utils::FixedArray<ThreadControl> thread_controls_;
  std::atomic<std::size_t> next_thread_idx_{0};
};

}  // namespace engine::ev

USERVER_NAMESPACE_END
