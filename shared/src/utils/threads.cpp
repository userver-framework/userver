#include <userver/utils/threads.hpp>

#include <sched.h>
#include <sys/param.h>

#ifdef __APPLE__
#include <pthread.h>
#include <sys/resource.h>
#elif defined(BSD)
#include <pthread_np.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <unistd.h>
#else
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include <algorithm>

#include <fmt/format.h>

#include <utils/check_syscall.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

namespace {
#ifdef __linux__
// Returns thread id
pid_t GetTid() { return static_cast<pid_t>(syscall(SYS_gettid)); }
#endif

struct LowPriorityParams {
  int policy;
  struct sched_param params;
};

LowPriorityParams QueryLowPriorityParams() {
  LowPriorityParams result{};

  utils::CheckSyscall(
      ::pthread_getschedparam(::pthread_self(), &result.policy, &result.params),
      "getting thread scheduling parameters");

  const auto min_priority = ::sched_get_priority_min(result.policy);
  const auto current_priority = result.params.sched_priority;

  result.params.sched_priority = (current_priority + min_priority - 1) / 2;
  if (current_priority == result.params.sched_priority) {
    throw std::runtime_error(fmt::format(
        "Failed to lower thread priority to {}: current thread "
        "priority is {}, minimal priority is {}",
        result.params.sched_priority, current_priority, min_priority));
  }

  return result;
}

}  // namespace

bool IsMainThread() {
#if defined(__APPLE__) || defined(BSD)
  return !!pthread_main_np();
#else
  return getpid() == GetTid();
#endif
}

void SetCurrentThreadIdleScheduling() {
#if defined(__APPLE__)
  static constexpr ::id_t kThisThread = 0;
  utils::CheckSyscall(
      ::setpriority(PRIO_DARWIN_THREAD, kThisThread, PRIO_DARWIN_BG),
      "setting scheduler to IDLE");
#elif defined(BSD)
  ::id_t kThisThread = ::getpid();
  utils::CheckSyscall(::setpriority(PRIO_PROCESS, kThisThread, 20),
                      "setting scheduler to IDLE");
#else
  static constexpr ::pid_t kThisThreadPid = 0;
  static constexpr struct sched_param kParam {};

  // Interesting fact from kernel/sched/core.c: "Treat SCHED_IDLE as nice 20"
  utils::CheckSyscall(::sched_setscheduler(kThisThreadPid, SCHED_IDLE, &kParam),
                      "setting scheduler to IDLE");
#endif
}

void SetCurrentThreadLowPriorityScheduling() {
  static const auto kLowPriority = QueryLowPriorityParams();

  utils::CheckSyscall(
      ::pthread_setschedparam(::pthread_self(), kLowPriority.policy,
                              &kLowPriority.params),
      "setting thread scheduling parameters");
}

}  // namespace utils

USERVER_NAMESPACE_END
