#include <engine/coro/stack_usage_monitor.hpp>

#include <coroutines/coroutine.hpp>

#include <engine/task/task_context.hpp>

// userfaultfd is linux-specific,
// and we use x86-64-specific RSP to calculate stack offsets
#if defined(__linux__) && defined(__x86_64__)
#include <sys/syscall.h>
// Linux kernel version check, basically
#if defined(__NR_userfaultfd)
#define HAS_STACK_USAGE_MONITOR
#endif
#endif

#ifdef HAS_STACK_USAGE_MONITOR

#include <fcntl.h>
#include <linux/userfaultfd.h>
#include <poll.h>
#include <sys/eventfd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <csignal>

#include <array>
#include <mutex>
#include <thread>
#include <utility>

#include <fmt/format.h>
#include <boost/container/small_vector.hpp>
#include <boost/stacktrace.hpp>

#include <userver/compiler/thread_local.hpp>
#include <userver/logging/log.hpp>
#include <userver/logging/stacktrace_cache.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/atomic.hpp>
#include <userver/utils/impl/userver_experiments.hpp>
#include <userver/utils/thread_name.hpp>
#include <utils/strerror.hpp>

#endif

namespace boost::coroutines2::detail {

struct FriendHijackTag final {};

template <>
class pull_coroutine<FriendHijackTag> {
 public:
  template <typename T>
  static const void* GetCbPtr(const push_coroutine<T>& coro) {
    return coro.cb_;
  }
};
}  // namespace boost::coroutines2::detail

USERVER_NAMESPACE_BEGIN

namespace engine::coro {

const void* GetCoroCbPtr(
    const boost::coroutines2::coroutine<impl::TaskContext*>::push_type&
        coro) noexcept {
  return boost::coroutines2::detail::pull_coroutine<
      boost::coroutines2::detail::FriendHijackTag>::GetCbPtr(coro);
}

#ifdef HAS_STACK_USAGE_MONITOR

namespace {

// SIGSTKFLT is defined, but not used by Linux.
// The name feels very fitting for what we do here.
constexpr auto kStackUsageSignal = SIGSTKFLT;

const auto kPageSize = utils::sys_info::GetPageSize();

// This value is basically something random >= MINSIGSTKSZ.
// It could be reduced, but given that we only mmap this much of
// memory per-thread, I wouldn't care too much.
const auto kAltStackSize = MINSIGSTKSZ * 2 + 16384;

// We don't need to log a stacktrace until this percent becomes somewhat high
// (symbolizing the stacktrace is far from free after all), but 70% of stack
// usage seems worrying enough to do that.
//
// If you plan to change this value, consider adjusting the marks distribution
// accordingly (see StackUsageMonitor::Impl::Register), since their resolution
// is ~15% of the stack.
constexpr std::uint16_t kStackUsagePctThresholdToLogStacktrace = 70;

std::uintptr_t RoundDownToPageSize(std::uintptr_t address) noexcept {
  return address & ~(kPageSize - 1);
}

std::uintptr_t RoundUpToPageSize(std::uintptr_t address) noexcept {
  return RoundDownToPageSize(address + kPageSize - 1);
}

std::uintptr_t GetStackBegin(const void* cb_ptr) noexcept {
  // So, cb_ptr is a pointer to push_coroutine<T>::control_block, which resides
  // at the beginning of the coroutine's stack.
  // Convenient! Just round it up (yes, up, stack is growing downwards) to the
  // page size.
  return RoundUpToPageSize(reinterpret_cast<std::uintptr_t>(cb_ptr));
}

[[noreturn]] __attribute__((noinline)) void
// the name is intentionally caps-ed, to be easily noticeable in the dumps
THE_COROUTINE_OVERFLOWED_ITS_STACK() {
  // We are hitting a coroutine stack overflow, which normally would result in a
  // SIGSEGV with some hard to diagnose coredump (basically, the only
  // way to diagnose it correctly as SO is to employ some arcane knowledge:
  // load the dump into debugger, find the coroutine stack pointer somewhere in
  // boost::coroutine2 internals, subtract RSP from it and compare the value
  // with stack_size, defined in the static_config/the default one).
  //
  // Instead, we are aborting with the function name as the last (or one
  // of the last) frame in the coredump. This should be a good-enough
  // diagnostics on its own, and we also can't do much better.
  std::abort();
}

constexpr std::size_t kMaxBinaryStacktraceSize = 2048;

struct StackUsageInfo final {
  // We check for this flag in TaskProcessor and reset it to false after
  // logging/accounting the stacktrace/usage_pct.
  bool actionable{false};
  std::uint16_t usage_pct{};
  // Binary representation of the stacktrace, populated by boost::stacktrace
  // (a bunch of RIPs, basically).
  std::array<char, kMaxBinaryStacktraceSize> serialized_stacktrace{};

  // We might want to add some task_ids here to log them even for
  // below-the-threshold usage.
  // Without these ids there's really no point in logging anything without the
  // stacktrace, since the logging is done from outside the coroutine and
  // there's no way to connect the dots.
  //
  // If you decide to add these ids, keep in mind that they have to be
  // async-signal-safe as well, so std::string wouldn't do, and a pre-allocated
  // buffer would be needed.
};

compiler::ThreadLocal stack_usage_info = [] { return StackUsageInfo{}; };

static void StackUsageHandler(const void* cb_ptr,
                              ucontext_t* stack_context) noexcept {
  // We calculate everything in pages, and here we round down because the stack
  // is growing downwards.
  const auto stack_pointer = RoundDownToPageSize(
      // This is the reason for x86-64 ifdef above. We could make it more
      // portable, but this is good enough for now, and it's easily adjustable.
      static_cast<std::size_t>(stack_context->uc_mcontext.gregs[REG_RSP]));

  const auto coro_stack_begin = GetStackBegin(cb_ptr);
  const auto stack_size = current_task::GetStackSize();

  // stack_pointer is guaranteed to be below the stack begin due to
  // a) stack growing downwards
  // b) how we set up the marks
  //
  // I _think_ ASAN's stack-use-after-return shenanigans with fake-stacks may
  // (may, I'm not really sure) interfere with all this userfaultfd-based
  // machinery, but even if they do the "b" invariant still holds.
  const auto stack_used = coro_stack_begin - stack_pointer;
  const auto stack_usage_pct = stack_used * 100 / stack_size;

  if (stack_usage_pct >= 99 || (stack_used >= stack_size - kPageSize)) {
    THE_COROUTINE_OVERFLOWED_ITS_STACK();
  }

  auto usage_info = stack_usage_info.Use();
  usage_info->usage_pct = stack_usage_pct;
  if (stack_usage_pct >= kStackUsagePctThresholdToLogStacktrace) {
    boost::stacktrace::safe_dump_to(usage_info->serialized_stacktrace.data(),
                                    kMaxBinaryStacktraceSize);
  }
  usage_info->actionable = true;
}

void StackUsageSignalHandler(int, siginfo_t*, void* context) noexcept {
  auto* current_task_coro_ptr = StackUsageMonitor::GetCurrentTaskCoroutine();
  if (!current_task_coro_ptr) {
    return;
  }

  StackUsageHandler(GetCoroCbPtr(**current_task_coro_ptr),
                    static_cast<ucontext_t*>(context));
}

int CreateUserfaultFd() {
  // there is no wrapper for userfaultfd in glibc, hence 'syscall' usage below.

#if defined(UFFD_USER_MODE_ONLY)
  const auto fd =
      ::syscall(__NR_userfaultfd, O_CLOEXEC | O_NONBLOCK | UFFD_USER_MODE_ONLY);
  if (fd == -1) {
    // UFFD_USER_MODE_ONLY is only available starting from 5.11 kernel,
    // and its usage results in EINVAL for older kernels.
    // We will retry without the flag in case of EINVAL.
    if (errno != EINVAL) {
      return -1;
    }
  }
#endif

  // Since 5.2 kernel this call could fail with EPERM in case of
  // "vm.unprivileged_userfaultfd" being set to false, but that we can't
  // control.
  return ::syscall(__NR_userfaultfd, O_CLOEXEC | O_NONBLOCK);
}

void LogWarningWithErrno(std::string_view message, bool limited = false,
                         logging::Level level = logging::Level::kWarning) {
  const auto saved_errno = errno;
  const auto message_with_errno = fmt::format(
      "{}, errno: {} ({})", message, saved_errno, utils::strerror(saved_errno));
  if (limited) {
    LOG_LIMITED(level) << message;
  } else {
    LOG(level) << message;
  }
}

class FdHolder final {
 public:
  FdHolder() = default;

  ~FdHolder() { Close(); }

  FdHolder(const FdHolder&) = delete;
  FdHolder(FdHolder&& other) noexcept : fd_{std::exchange(other.fd_, -1)} {}

  void Reset(int fd) {
    Close();
    fd_ = fd;
  }
  int Get() const { return fd_; }

  void Close() {
    const auto fd = Get();
    if (fd != -1) {
      ::close(fd);
    }

    fd_ = -1;
  }

 private:
  int fd_{-1};
};

}  // namespace

class StackUsageMonitor::Impl final {
 public:
  explicit Impl(std::size_t coro_stack_size)
      : coro_stack_size_{coro_stack_size} {
    UASSERT(coro_stack_size % kPageSize == 0);
  }
  ~Impl() { Stop(); }

  void Start() {
    if (!utils::impl::kCoroutineStackUsageMonitorExperiment.IsEnabled()) {
      LOG_INFO() << "StackUsageMonitor is not enabled, skipping initialization";
      return;
    }

    const auto monitor_fd = CreateUserfaultFd();
    if (monitor_fd == -1) {
      LogWarningWithErrno(
          "Failed to initialize StackUsageMonitor(userfaultfd)");
      // Here and below we just return without closing the fds, since we don't
      // care enough to complicate the code.
      // They will be closed in `Stop` anyway.
      return;
    }
    monitor_fd_.Reset(monitor_fd);

    const auto stop_fd = ::eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    if (stop_fd == -1) {
      // This shouldn't really happen, but still
      LogWarningWithErrno("Failed to initialize StackUsageMonitor(eventfd)");
      return;
    }
    stop_fd_.Reset(stop_fd);

    decltype(uffdio_api::features) supported_features{};
    {
      uffdio_api api{};
      api.api = UFFD_API;
      api.features = UFFD_FEATURE_THREAD_ID;
      if (::ioctl(monitor_fd_.Get(), UFFDIO_API, &api) == -1) {
        LogWarningWithErrno("Failed to initialize StackUsageMonitor(UFFD_API)");
        return;
      }
      supported_features = api.features;

      // We need UFFD_FEATURE_THREAD_ID for out signal-based monitoring to work
      // (otherwise we wouldn't know to which thread the signal should be sent).
      //
      // The ::ioctl above should fail with EINVAL if kernel doesn't support the
      // UFFD_FEATURE_THREAD_ID feature, so this condition shouldn't ever be
      // satisfied, but better safe than sorry.
      if (!(supported_features & UFFD_FEATURE_THREAD_ID)) {
        LogWarningWithErrno(
            "Failed to initialize StackUsageMonitor(UFFD_FEATURE_THREAD_ID)");
        return;
      }
    }

    // We are all set, start the monitoring thread
    monitor_thread_ = std::thread{[this] { MonitorForPageFaults(); }};
    is_active_ = true;

    LOG_INFO() << "Successfully initialized StackUsageMonitor, kernel supports "
                  "following userfaultfd features: "
               << supported_features;
  }

  void Cleanup() noexcept {
    stop_fd_.Close();

    // Quoting the docs:
    // "When the last file descriptor referring to a userfaultfd object
    // is closed, all memory ranges that were registered with the object
    // are unregistered and unread events are flushed."
    //
    // We only have a single file descriptor, so when the close below completes,
    // our page faults handling is effectively finished, and the kernel would
    // continue to resolve the faults as usual.
    // This is very important, because it gives us a possibility to break the
    // reader loop at any moment (in case of unexpected errors, for example)
    // without worrying about unhandled event left in the FD, which would
    // otherwise stall the faulting threads forever.
    monitor_fd_.Close();
  }

  void Stop() noexcept {
    // Notify the reader loop about termination.
    // The loop itself will close all the fds, or they'll be closed in
    // destructors anyway.
    const auto stop_fd = stop_fd_.Get();
    if (stop_fd != -1) {
      constexpr std::uint64_t kSomeValue{1};
      do {
        [[maybe_unused]] const auto written = ::write(stop_fd, &kSomeValue, 8);
      } while (errno == EINTR);
    }

    if (monitor_thread_.joinable()) {
      monitor_thread_.join();
    }

    for (auto* thread_alt_stack : threads_alt_stacks) {
      ::munmap(thread_alt_stack, kAltStackSize);
    }

    is_active_ = false;
  }

  void Register(const void* cb_ptr) {
    if (!is_active_) {
      return;
    }

    const auto stack_begin = GetStackBegin(cb_ptr);
    const auto stack_pages_count = coro_stack_size_ / kPageSize;

    const auto add_stack_usage_mark = [this, stack_begin, stack_pages_count](
                                          std::size_t usage_pct) {
      const auto mark_offset =
          kPageSize * (stack_pages_count * usage_pct / 100);
      // Some sanity checks
      if (usage_pct > 100 || stack_pages_count < 1 || mark_offset == 0) {
        return;
      }

      uffdio_register reg{};
      // Stack is growing downwards, but the range is upwards,
      // mark the page where mark belongs, not the previous one.
      // We also need to round this to page size anyway.
      reg.range.start = RoundDownToPageSize(stack_begin - mark_offset);
      reg.range.len = kPageSize;
      reg.mode = UFFDIO_REGISTER_MODE_MISSING;
      if (ioctl(monitor_fd_.Get(), UFFDIO_REGISTER, &reg) == -1) {
        LogWarningWithErrno("Failed to register a coroutine stack usage mark",
                            /* limited = */ true);
      }
    };

    // Just a bunch of reasonably scattered marks, could be changed.
    // Don't forget to adjust `kStackUsagePctThresholdToLogStacktrace` as well
    // if you do so.
    for (std::size_t usage_pct = 15; usage_pct <= 90; usage_pct += 15) {
      // 15, 30, 45, 60, 75, 90
      add_stack_usage_mark(usage_pct);
    }

    // Note that mprotect takes precedence over userfaultfd, and this page is
    // NOT mprotect-ed, but the very next one is (courtesy of
    // boost::coroutines2::protected_fixedsize_stack).
    add_stack_usage_mark(100);
  }

  void RegisterThread() {
    void* alt_stack = ::mmap(nullptr, kAltStackSize, PROT_READ | PROT_WRITE,
                             MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (alt_stack == MAP_FAILED) {
      // Don't register the thread at all, since we don't have a stack to handle
      // a potential StackOverflow.
      // This shouldn't really happen.
      return;
    }

    // Touch the thread_local data to force its initialization here and not in
    // the signal handler. Not that it should matter, just for a good measure.
    {
      auto usage_info = stack_usage_info.Use();
      usage_info->actionable = false;
    }

    // Now we inform the kernel about the alt-stack presence ...
    stack_t ss{};
    ss.ss_sp = alt_stack;
    ss.ss_size = kAltStackSize;
    ss.ss_flags = 0;
    if (sigaltstack(&ss, nullptr) == -1) {
      LogWarningWithErrno(
          "Failed to set up an alt-stack for TaskProcessor "
          "thread(sigaltstack)");
      ::munmap(alt_stack, kAltStackSize);
      return;
    }

    // ... and use the alt-stack to handle our monitoring signal.
    struct sigaction sa {};
    sa.sa_flags = SA_ONSTACK | SA_SIGINFO;
    sa.sa_sigaction = &StackUsageSignalHandler;
    if (sigaction(kStackUsageSignal, &sa, nullptr) == -1) {
      LogWarningWithErrno(
          "Failed to set up an alt-stack for TaskProcessor "
          "thread(sigaction)");
      ::munmap(alt_stack, kAltStackSize);
      return;
    }

    // Iff the alt-stack initialization succeeded, we add the tid->pthread_t
    // mapping into 'thread_id_to_pthread_id_', which would lead to our
    // signal-based monitoring machinery kicking in.
    //
    // Otherwise, nothing happens, and we just resolve the page-fault via
    // userfaultfd itself.
    {
      const auto tid = ::syscall(SYS_gettid);
      const auto thread_id = ::pthread_self();

      const std::lock_guard lock{tid_to_pthread_initialization_mutex_};
      thread_id_to_pthread_id_.emplace_back(tid, thread_id);
      threads_alt_stacks.push_back(alt_stack);
    }
  }

  void AccountStackUsage() {
    UASSERT(!current_task::GetCurrentTaskContextUnchecked());

    auto usage_info = stack_usage_info.Use();
    if (!usage_info->actionable) {
      return;
    }

    const auto usage_pct = usage_info->usage_pct;
    // There is the same condition in the signal-handler of ours, if it doesn't
    // hold then there's no stacktrace to log.
    // And if there's no stacktrace to log, there's not much point in logging
    // anything at all, metrics should be enough.
    if (usage_pct >= kStackUsagePctThresholdToLogStacktrace) {
      fmt::memory_buffer buff{};
      fmt::format_to(std::back_inserter(buff),
                     "Coroutine is using approximately {}% of its stack\n",
                     usage_pct);
      const auto coro_stack_usage_message =
          std::string_view{buff.data(), buff.size()};
      const auto stacktrace = boost::stacktrace::stacktrace::from_dump(
          usage_info->serialized_stacktrace.data(), kMaxBinaryStacktraceSize);

      const auto readable_stacktrace =
          logging::stacktrace_cache::to_string(stacktrace);
      LOG_WARNING() << coro_stack_usage_message << readable_stacktrace;
    }

    utils::AtomicMax(max_stack_usage_pct_, usage_info->usage_pct);

    usage_info->actionable = false;
  }

  std::uint16_t GetMaxStackUsagePct() const {
    return max_stack_usage_pct_.load();
  }

  bool IsActive() const { return is_active_; }

 private:
  void MonitorForPageFaults() {
    utils::SetCurrentThreadName("stack-usage-monitor");

    UASSERT(monitor_fd_.Get() != -1);
    UASSERT(stop_fd_.Get() != -1);
    std::array<::pollfd, 2> poll_fds{::pollfd{monitor_fd_.Get(), POLLIN, 0},
                                     ::pollfd{stop_fd_.Get(), POLLIN, 0}};

    while (true) {
      poll_fds[0].revents = poll_fds[1].revents = 0;

      const auto events_count = ::poll(poll_fds.data(), poll_fds.size(), -1);
      if (events_count == -1) {
        if (errno == EINTR) {
          continue;
        }
        // We are done here.
        break;
      }
      if (poll_fds[1].revents) {
        // stop was requested
        break;
      }

      uffd_msg message{};
      const auto nread = ::read(monitor_fd_.Get(), &message, sizeof(message));
      if (nread == 0) {
        // This is something very strange, but could happen I guess (some
        // spurious poll wakeup or whatever). Just continue the loop.
        continue;
      }
      if (nread == -1) {
        // Docs say this can only happen if the userfaultfd has not been
        // initialized correctly, and then the read fails with EINVAL.
        // Shouldn't happen, but still.
        LogWarningWithErrno("Failed to read an userfaultfd event", false,
                            logging::Level::kError);
        break;
      }

      if (message.event != UFFD_EVENT_PAGEFAULT) {
        // We are not interested in any other kind of events
        continue;
      }

      const auto faulting_thread_id =
          PidToPthreadT(message.arg.pagefault.feat.ptid);
      const bool wakeup_by_signal = faulting_thread_id.has_value();

      uffdio_zeropage page{};
      page.range.start = RoundDownToPageSize(message.arg.pagefault.address);
      page.range.len = kPageSize;
      // If the faulting thread is a TaskProcessor thread, we will wake it up by
      // the "stack-usage-monitor" signal later
      page.mode = wakeup_by_signal ? UFFDIO_ZEROPAGE_MODE_DONTWAKE : 0;
      if (::ioctl(monitor_fd_.Get(), UFFDIO_ZEROPAGE, &page)) {
        // This REALLY shouldn't happen.
        // If it does, we can't do anything but close the userfaultfd and hope
        // that kernel resolves the fault at its own (which it should, I
        // believe), even thought we've already read the event.
        LogWarningWithErrno("Failed to resolve a page-fault", false,
                            logging::Level::kError);
        break;
      }

      if (wakeup_by_signal) {
        if (pthread_kill(*faulting_thread_id, kStackUsageSignal)) {
          // Yikes. Close the userfaultfd, hope for the best.
          LogWarningWithErrno("Failed to wakeup the faulting thread by signal",
                              false, logging::Level::kError);
          break;
        }
      }
    }

    Cleanup();
  }

  std::optional<pthread_t> PidToPthreadT(int ptid) const {
    for (const auto& [pid, pthread_id] : thread_id_to_pthread_id_) {
      if (ptid == pid) return pthread_id;
    }

    return std::nullopt;
  }

  // We only need to protect the initialization of the mapping, since it doesn't
  // change at runtime.
  std::mutex tid_to_pthread_initialization_mutex_;
  boost::container::small_vector<std::pair<int, pthread_t>, 32>
      thread_id_to_pthread_id_{};
  boost::container::small_vector<void*, 32> threads_alt_stacks{};

  std::size_t coro_stack_size_;
  std::thread monitor_thread_;
  FdHolder monitor_fd_{};
  FdHolder stop_fd_{};
  bool is_active_{false};

  std::atomic<std::uint16_t> max_stack_usage_pct_{0};
};

#else

class StackUsageMonitor::Impl final {
 public:
  explicit Impl(std::size_t) {}

  void Start() {}
  void Stop() {}

  void Register(const void*) {}

  void RegisterThread() {}

  void AccountStackUsage() {}

  std::uint16_t GetMaxStackUsagePct() const noexcept { return 0; }
  bool IsActive() const noexcept { return false; }
};

#endif

StackUsageMonitor::StackUsageMonitor(std::size_t coro_stack_size)
    : impl_{coro_stack_size} {}

StackUsageMonitor::~StackUsageMonitor() { Stop(); }

void StackUsageMonitor::Start() { impl_->Start(); }

void StackUsageMonitor::Stop() { impl_->Stop(); }

void StackUsageMonitor::Register(
    const boost::coroutines2::coroutine<impl::TaskContext*>::push_type& coro) {
  impl_->Register(GetCoroCbPtr(coro));
}

void StackUsageMonitor::RegisterThread() { impl_->RegisterThread(); }

impl::CountedCoroutinePtr*
StackUsageMonitor::GetCurrentTaskCoroutine() noexcept {
  auto* current_task = current_task::GetCurrentTaskContextUnchecked();
  if (!current_task || !current_task->GetCoroutinePtr()) {
    return nullptr;
  }

  return &current_task->GetCoroutinePtr();
}

void StackUsageMonitor::AccountStackUsage() { impl_->AccountStackUsage(); }

std::uint16_t StackUsageMonitor::GetMaxStackUsagePct() const noexcept {
  return impl_->GetMaxStackUsagePct();
}

bool StackUsageMonitor::IsActive() const noexcept { return impl_->IsActive(); }

bool StackUsageMonitor::DebugCanUseUserfaultfd() {
#ifndef HAS_STACK_USAGE_MONITOR
  return false;
#else
  const auto fd = CreateUserfaultFd();
  if (fd == -1) {
    return false;
  }

  ::close(fd);
  return true;
#endif
}

// We need this to be noinline for __builtin_frame_address to work somewhat
// reliably
__attribute__((noinline)) std::size_t GetCurrentTaskStackUsageBytes() noexcept {
  auto* current_task_context = current_task::GetCurrentTaskContextUnchecked();
  if (!current_task_context || !current_task_context->GetCoroutinePtr()) {
    return 0;
  }

  auto& coro = current_task_context->GetCoroutinePtr();
  const auto* coro_stack_begin = GetCoroCbPtr(*coro);

  const auto stack_pointer = __builtin_frame_address(0);
  return reinterpret_cast<std::uintptr_t>(coro_stack_begin) -
         reinterpret_cast<std::uintptr_t>(stack_pointer);
}

}  // namespace engine::coro

USERVER_NAMESPACE_END
