#include <logging/log.hpp>

#include <atomic>
#include <iostream>

#include <boost/container/small_vector.hpp>
#include <boost/exception/diagnostic_information.hpp>

#include <engine/run_in_coro.hpp>
#include <engine/task/task_context.hpp>
#include <logging/log_extra_stacktrace.hpp>
#include <logging/spdlog.hpp>
#include <rcu/rcu.hpp>
#include <tracing/span.hpp>
#include <utils/assert.hpp>
#include <utils/string_view.hpp>
#include <utils/traceful_exception.hpp>
#include "log_workaround.hpp"

#include <boost/stacktrace/detail/to_dec_array.hpp>
#include <boost/stacktrace/detail/to_hex_array.hpp>

#include "log_helper_impl.hpp"

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
    : pimpl_(std::move(logger), level) {
  [[maybe_unused]] const auto initial_capacity = pimpl_->Capacity();

  path = StripPathBase(path);

  // The following functions actually never throw if the assertions at the
  // bottom hold.
  if (mode != Mode::kNoSpan) {
    NOTHROW_CALL_CONSTRUCTOR(path, line, LogSpan())
  }
  NOTHROW_CALL_CONSTRUCTOR(path, line, LogModule(path, line, func))
  NOTHROW_CALL_CONSTRUCTOR(path, line, LogIds())

  LogTextKey();  // This member outputs only a key without value
                 // This call must be the last in constructor

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

LogHelper::~LogHelper() { DoLog(); }

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
  const auto& items = extra_.extra_;
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
  uint64_t coro_id = task ? task->GetCoroId() : 0;
  auto thread_id = reinterpret_cast<void*>(pthread_self());

  Put(utils::encoding::kTskvPairsSeparator);
  Put("task_id");
  Put(utils::encoding::kTskvKeyValueSeparator);
  PutHexShort(task_id);

  Put(utils::encoding::kTskvPairsSeparator);
  Put("coro_id");
  Put(utils::encoding::kTskvKeyValueSeparator);
  PutHexShort(coro_id);

  Put(utils::encoding::kTskvPairsSeparator);
  Put("thread_id");
  Put(utils::encoding::kTskvKeyValueSeparator);
  PutHex(thread_id);
}

void LogHelper::LogSpan() {
  auto* span = tracing::Span::CurrentSpanUnchecked();
  if (span) *this << *span;
}

// TODO: use std::to_chars in all the Put* functions.
void LogHelper::PutHexShort(unsigned long long value) {
  using boost::stacktrace::detail::to_hex_array_bytes;
  if (value == 0) {
    Put('0');
    return;
  }

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
  std::array<char, sizeof(value) * 2> ret;
  char* out = ret.data() + ret.size();
  while (value) {
    --out;
    *out = to_hex_array_bytes[value & 0xFu];
    value >>= 4;
  }

  Put({out, static_cast<std::size_t>(ret.data() + ret.size() - out)});
}
void LogHelper::PutHex(const void* value) {
  Put(boost::stacktrace::detail::to_hex_array(value).data());
}
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
  Put(boost::stacktrace::detail::to_dec_array(value).data());
}
void LogHelper::PutSigned(long long value) {
  auto value_as_unsigned = static_cast<unsigned long long>(value);

  if (value < 0) {
    Put('-');

    // safe way to convert negative value to a positive
    value_as_unsigned = 0u - value_as_unsigned;
  }

  PutUnsigned(value_as_unsigned);
}

void LogHelper::Put(utils::string_view value) {
  pimpl_->xsputn(value.data(), value.size());
}

void LogHelper::Put(char value) { pimpl_->xsputn(&value, 1); }

void LogHelper::PutException(const std::exception& ex) {
  const auto* traceful = dynamic_cast<const utils::TracefulExceptionBase*>(&ex);
  if (traceful) {
    const auto& message_buffer = traceful->MessageBuffer();
    Put(utils::string_view(message_buffer.data(), message_buffer.size()));
    extra_.Extend(impl::MakeLogExtraStacktrace(
        traceful->Trace(), impl::LogExtraStacktraceFlags::kFrozen));
  } else {
    Put(ex.what());
  }
}

LogHelper::EncodingGuard::EncodingGuard(LogHelper& lh, Encode mode) noexcept
    : lh{lh} {
  lh.pimpl_->SetEncoding(mode);
}

LogHelper::EncodingGuard::~EncodingGuard() {
  lh.pimpl_->SetEncoding(Encode::kNone);
}

void LogFlush() { DefaultLogger()->flush(); }

}  // namespace logging
