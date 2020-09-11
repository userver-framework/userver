#include <logging/log.hpp>

#include <atomic>
#include <iostream>
#include <type_traits>
#include <vector>

#include <boost/exception/diagnostic_information.hpp>

#include <engine/run_in_coro.hpp>
#include <engine/task/task_context.hpp>
#include <logging/log_extra_stacktrace.hpp>
#include <logging/spdlog.hpp>
#include <rcu/rcu.hpp>
#include <tracing/span.hpp>
#include <utils/assert.hpp>
#include <utils/datetime.hpp>
#include <utils/traceful_exception.hpp>

#include <logging/log_helper_impl.hpp>
#include <logging/log_workaround.hpp>

#define NOTHROW_CALL_BASE(ERROR_PREFIX, FUNCTION)                             \
  try {                                                                       \
    FUNCTION;                                                                 \
  } catch (...) {                                                             \
    try {                                                                     \
      std::cerr << ERROR_PREFIX "failed to " #FUNCTION ":"                    \
                << boost::current_exception_diagnostic_information() << '\n'; \
      UASSERT_MSG(false, #FUNCTION);                                          \
    } catch (...) {                                                           \
      UASSERT_MSG(false, #FUNCTION " (second catch)");                        \
    }                                                                         \
  }

#define NOTHROW_CALL_CONSTRUCTOR(PATH, LINE, FUNCTION) \
  NOTHROW_CALL_BASE((PATH) << ':' << (LINE) << ": ", (FUNCTION))

#define NOTHROW_CALL_GENERIC(FUNCTION) NOTHROW_CALL_BASE("LogHelper ", FUNCTION)

namespace logging {
namespace {

constexpr char kPathLineSeparator = ':';

class LogExtraValueVisitor final {
 public:
  LogExtraValueVisitor(LogHelper& lh) noexcept : lh_(lh) {}

  template <typename T>
  void operator()(const T& value) const {
    lh_ << value;
  }

 private:
  LogHelper& lh_;
};

auto& DefaultLoggerInternal() {
  static rcu::Variable<LoggerPtr> default_logger_ptr(
      MakeStderrLogger("default"));
  return default_logger_ptr;
}

bool LoggerShouldLog(LoggerPtr logger, Level level) {
  return logger->should_log(static_cast<spdlog::level::level_enum>(level));
}

void UpdateLogLevelCache() {
  const auto& logger = DefaultLogger();
  for (int i = 0; i < kLevelMax + 1; i++)
    GetShouldLogCache()[i] = LoggerShouldLog(logger, static_cast<Level>(i));

  GetShouldLogCache()[static_cast<int>(Level::kNone)] = false;
}

// For IDE parsing
#if !defined(LOG_PATH_BASE)
#define LOG_PATH_BASE ""
#endif

const char* StripPathBase(const char* path) {
  static const char* kLogPathBase = LOG_PATH_BASE;

  auto* base_ptr = kLogPathBase;
  auto* path_stripped = path;
  while (*base_ptr == *path_stripped && *base_ptr) {
    ++base_ptr;
    ++path_stripped;
  }
  return *base_ptr ? path : path_stripped;
}

template <typename T>
class ThreadLocalMemPool {
 public:
  static constexpr size_t kMaxSize = 1000;

  template <typename... Args>
  static std::unique_ptr<T> Pop(Args&&... args) {
    auto& pool = GetPool();
    if (pool.empty()) {
      // https://bugs.llvm.org/show_bug.cgi?id=38176
      // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDeleteLeaks)
      return std::make_unique<T>(std::forward<Args>(args)...);
    }

    auto& raw = pool.back();
    // if ctor throws, memory remains in pool
    new (raw.get()) T(std::forward<Args>(args)...);
    // arm dtor, transfer ownership (noexcept)
    std::unique_ptr<T> obj(reinterpret_cast<T*>(raw.release()));
    // prune pool
    pool.pop_back();
    return obj;
  }

  static void Push(std::unique_ptr<T> obj) {
    auto& pool = GetPool();
    if (pool.size() >= kMaxSize) return;

    // disarm dtor, transfer ownership (noexcept)
    std::unique_ptr<StorageType> raw(
        reinterpret_cast<StorageType*>(obj.release()));
    // call dtor (might throw)
    reinterpret_cast<T*>(raw.get())->~T();
    // store into pool (may throw on container allocation)
    pool.push_back(std::move(raw));
  }

 private:
  using StorageType = std::aligned_storage_t<sizeof(T), alignof(T)>;

  static auto& GetPool() {
    thread_local std::vector<std::unique_ptr<StorageType>> pool_;
    return pool_;
  }
};

constexpr bool NeedsQuoteEscaping(char c) { return c == '\"' || c == '\\'; }

constexpr bool IsPowerOf2(uint64_t n) { return (n & (n - 1)) == 0; }

}  // namespace

LoggerPtr DefaultLogger() { return DefaultLoggerInternal().ReadCopy(); }

LoggerPtr SetDefaultLogger(LoggerPtr logger) {
  if (engine::current_task::GetCurrentTaskContextUnchecked() == nullptr) {
    RunInCoro([&logger] { logger = SetDefaultLogger(logger); }, 1);
    return logger;
  }

  auto ptr = DefaultLoggerInternal().StartWrite();
  swap(*ptr, logger);
  ptr.Commit();

  UpdateLogLevelCache();
  return logger;
}

void SetDefaultLoggerLevel(Level level) {
  DefaultLogger()->set_level(static_cast<spdlog::level::level_enum>(level));
  UpdateLogLevelCache();
}

Level GetDefaultLoggerLevel() {
  return static_cast<Level>(DefaultLogger()->level());
}

std::ostream& LogHelper::Stream() { return pimpl_->Stream(); }

LogHelper::LogHelper(LoggerPtr logger, Level level, const char* path, int line,
                     const char* func, Mode mode) noexcept
    : pimpl_(ThreadLocalMemPool<Impl>::Pop(std::move(logger), level)) {
  [[maybe_unused]] const auto initial_capacity = pimpl_->Capacity();

  path = StripPathBase(path);

  // The following functions actually never throw if the assertions at the
  // bottom hold.
  if (mode != Mode::kNoSpan) {
    NOTHROW_CALL_CONSTRUCTOR(path, line, LogSpan())
  }
  NOTHROW_CALL_CONSTRUCTOR(path, line, LogModule(path, line, func))
  NOTHROW_CALL_CONSTRUCTOR(path, line, LogIds())

  LogTextKey();
  pimpl_->MarkTextBegin();
  // Must not log further system info after this point

  UASSERT_MSG(
      !pimpl_->IsStreamInitialized(),
      "Some function frome above initialized the std::ostream. That's a "
      "heavy operation that should be avoided. Add a breakpoint on Stream() "
      "function and tune the implementation.");

  UASSERT_MSG(
      initial_capacity == pimpl_->Capacity(),
      "Logging buffer is too small to keep initial data. Adjust the "
      "spdlog::details::log_msg class or reduce the output of the above "
      "functions.");
}

LogHelper::~LogHelper() {
  DoLog();
  ThreadLocalMemPool<Impl>::Push(std::move(pimpl_));
}

constexpr size_t kSizeLimit = 10000;

bool LogHelper::IsLimitReached() const {
  return pimpl_->TextSize() >= kSizeLimit;
}

void LogHelper::DoLog() noexcept {
  NOTHROW_CALL_GENERIC(AppendLogExtra())
  if (pimpl_->IsStreamInitialized()) {
    NOTHROW_CALL_GENERIC(Stream().flush());
  }

  try {
    impl::SinkMessage(pimpl_->GetLogger(), pimpl_->Message());
  } catch (...) {
    try {
      std::cerr << "LogHelper failed to sink_it_:"
                << boost::current_exception_diagnostic_information() << '\n';

      NOTHROW_CALL_GENERIC(std::cerr << pimpl_->StreamBuf());
    } catch (...) {
    }
    UASSERT_MSG(false, "LogHelper::DoLog()");
  }
}

void LogHelper::AppendLogExtra() {
  const auto& items = pimpl_->GetLogExtra().extra_;
  if (items->empty()) return;

  LogExtraValueVisitor visitor(*this);

  for (const auto& item : *items) {
    Put(utils::encoding::kTskvPairsSeparator);

    {
      EncodingGuard guard{*this, Encode::kKeyReplacePeriod};
      Put(item.first);
    }
    Put(utils::encoding::kTskvKeyValueSeparator);
    std::visit(visitor, item.second.GetValue());
  }
}

void LogHelper::LogTextKey() {
  Put(utils::encoding::kTskvPairsSeparator);
  Put("text");
  Put(utils::encoding::kTskvKeyValueSeparator);
}

void LogHelper::LogModule(const char* path, int line, const char* func) {
  Put("module");
  Put(utils::encoding::kTskvKeyValueSeparator);
  Put(func);
  Put(" ( ");
  Put(path);
  Put(kPathLineSeparator);
  PutSigned(line);
  Put(" ) ");
}

void LogHelper::LogIds() {
  auto task = engine::current_task::GetCurrentTaskContextUnchecked();
  uint64_t task_id = task ? reinterpret_cast<uint64_t>(task) : 0;
  auto thread_id = reinterpret_cast<void*>(pthread_self());

  Put(utils::encoding::kTskvPairsSeparator);
  Put("task_id");
  Put(utils::encoding::kTskvKeyValueSeparator);
  *this << HexShort{task_id};

  Put(utils::encoding::kTskvPairsSeparator);
  Put("thread_id");
  Put(utils::encoding::kTskvKeyValueSeparator);
  *this << Hex{thread_id};
}

void LogHelper::LogSpan() {
  auto* span = tracing::Span::CurrentSpanUnchecked();
  if (span) *this << *span;
}

LogHelper& LogHelper::operator<<(const LogExtra& extra) {
  pimpl_->GetLogExtra().Extend(extra);
  return *this;
}

LogHelper& LogHelper::operator<<(LogExtra&& extra) {
  pimpl_->GetLogExtra().Extend(std::move(extra));
  return *this;
}

// TODO: use std::to_chars in all the Put* functions.
void LogHelper::PutFloatingPoint(float value) {
  format_to(pimpl_->Message().raw, "{}", value);
}
void LogHelper::PutFloatingPoint(double value) {
  format_to(pimpl_->Message().raw, "{}", value);
}
void LogHelper::PutFloatingPoint(long double value) {
  format_to(pimpl_->Message().raw, "{}", value);
}
void LogHelper::PutUnsigned(unsigned long long value) {
  format_to(pimpl_->Message().raw, "{}", value);
}
void LogHelper::PutSigned(long long value) {
  format_to(pimpl_->Message().raw, "{}", value);
}

LogHelper& LogHelper::operator<<(Hex hex) {
  format_to(pimpl_->Message().raw, "0x{:016X}", hex.value);
  return *this;
}

LogHelper& LogHelper::operator<<(HexShort hex) {
  format_to(pimpl_->Message().raw, "{:X}", hex.value);
  return *this;
}

void LogHelper::Put(std::string_view value) {
  pimpl_->xsputn(value.data(), value.size());
}

void LogHelper::Put(char value) { pimpl_->xsputn(&value, 1); }

void LogHelper::PutException(const std::exception& ex) {
  if (!ShouldLog(Level::kDebug)) {
    Put(ex.what());
    return;
  }

  const auto* traceful = dynamic_cast<const utils::TracefulExceptionBase*>(&ex);
  if (traceful) {
    const auto& message_buffer = traceful->MessageBuffer();
    Put(std::string_view(message_buffer.data(), message_buffer.size()));
    pimpl_->GetLogExtra().Extend(impl::MakeLogExtraStacktrace(
        traceful->Trace(), impl::LogExtraStacktraceFlags::kFrozen));
  } else {
    Put(ex.what());
  }
}

void LogHelper::PutQuoted(std::string_view value) {
  constexpr size_t kQuotesSize = 2;

  const auto old_message_size = pimpl_->TextSize();
  const auto allowed_size =
      std::max(kSizeLimit, old_message_size + kQuotesSize) -
      (old_message_size + kQuotesSize);
  size_t used_size = 0;

  Put('\"');

  for (const char c : value) {
    const bool needs_escaping = NeedsQuoteEscaping(c);
    const size_t escaped_size = needs_escaping ? 2 : 1;

    used_size += escaped_size;
    if (used_size > allowed_size) break;

    if (needs_escaping) {
      Put('\\');
    }
    Put(c);
  }

  if (used_size > allowed_size) {
    Put("...");
  }

  Put('\"');
}

LogHelper::EncodingGuard::EncodingGuard(LogHelper& lh, Encode mode) noexcept
    : lh{lh} {
  lh.pimpl_->SetEncoding(mode);
}

LogHelper::EncodingGuard::~EncodingGuard() {
  lh.pimpl_->SetEncoding(Encode::kNone);
}

LogHelper& operator<<(LogHelper& lh, std::chrono::system_clock::time_point tp) {
  lh << utils::datetime::Timestring(tp);
  return lh;
}

void LogFlush() { DefaultLogger()->flush(); }

bool impl::RateLimiter::ShouldLog(Level level) {
  if (!logging::ShouldLog(level)) return false;

  constexpr auto kResetInterval =
      std::chrono::steady_clock::duration{std::chrono::seconds{1}};
  const auto now = std::chrono::steady_clock::now();

  if (now - last_reset_time_ >= kResetInterval) {
    count_since_reset_ = 0;
    last_reset_time_ = now;
  }
  return IsPowerOf2(++count_since_reset_);
}

}  // namespace logging
