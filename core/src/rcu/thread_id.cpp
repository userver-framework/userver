#include <rcu/thread_id.hpp>

#include <atomic>
#include <iostream>

#include <boost/lockfree/queue.hpp>
#include <boost/stacktrace.hpp>

#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

namespace rcu::impl {

namespace {

class ThreadIdManager final {
 public:
  ThreadId AcquireThreadId() noexcept {
    ThreadId free_id{};

    // NOLINTNEXTLINE(clang-analyzer-core.UndefinedBinaryOperatorResult)
    if (free_thread_ids.pop(free_id)) return free_id;

    const auto id = thread_count.fetch_add(1, std::memory_order_release);
    if (id >= kMaxThreadCount) {
      auto trace = boost::stacktrace::stacktrace();
      std::cerr << "Too many threads working with RCU. Limit is "
                << kMaxThreadCount << ". Stacktrace:\n"
                << trace << "\n";
      logging::LogFlush();
      std::abort();
    }

    return id;
  }

  void ReleaseThreadId(ThreadId id) noexcept { free_thread_ids.push(id); }

  ThreadId GetThreadCount() const noexcept {
    return thread_count.load(std::memory_order_acquire);
  }

 private:
  std::atomic<ThreadId> thread_count{0};
  boost::lockfree::queue<ThreadId> free_thread_ids{0};
};

ThreadIdManager& GetThreadIdManager() noexcept {
  static ThreadIdManager instance;
  return instance;
}

class ThreadIdScope final {
 public:
  // NOLINTNEXTLINE(clang-analyzer-core.UndefinedBinaryOperatorResult)
  ThreadIdScope() noexcept : id_(GetThreadIdManager().AcquireThreadId()) {}
  ~ThreadIdScope() { GetThreadIdManager().ReleaseThreadId(id_); }

  ThreadIdScope(ThreadIdScope&&) = delete;
  ThreadIdScope& operator=(ThreadIdScope&&) = delete;

  ThreadId GetId() const noexcept { return id_; }

 private:
  ThreadId id_;
};

}  // namespace

ThreadId GetThreadCount() noexcept {
  return GetThreadIdManager().GetThreadCount();
}

ThreadId GetCurrentThreadId() noexcept {
  // NOLINTNEXTLINE(clang-analyzer-core.UndefinedBinaryOperatorResult)
  thread_local const ThreadIdScope scope;
  return scope.GetId();
}

}  // namespace rcu::impl

USERVER_NAMESPACE_END
