#include <logging/log.hpp>

#include <atomic>
#include <iostream>

#include <boost/container/small_vector.hpp>

#include <engine/task/task_context.hpp>
#include <logging/spdlog.hpp>
#include <tracing/span.hpp>
#include <utils/swappingsmart.hpp>
#include "log_streambuf.hpp"
#include "log_workaround.hpp"

#define NOTHROW_CALL_BASE(ERROR_PREFIX, FUNCTION)                             \
  try {                                                                       \
    FUNCTION;                                                                 \
  } catch (...) {                                                             \
    try {                                                                     \
      std::cerr << ERROR_PREFIX "failed to " #FUNCTION ":"                    \
                << boost::current_exception_diagnostic_information() << '\n'; \
      assert(false && #FUNCTION);                                             \
    } catch (...) {                                                           \
      assert(false && #FUNCTION " (second catch)");                           \
    }                                                                         \
  }

#define NOTHROW_CALL_CONSTRUCTOR(PATH, LINE, FUNCTION) \
  NOTHROW_CALL_BASE(PATH << ':' << LINE << ": ", FUNCTION)

#define NOTHROW_CALL_GENERIC(FUNCTION) NOTHROW_CALL_BASE("LogHelper ", FUNCTION)

namespace logging {
namespace {

const char kPathLineSeparator = ':';

class LogExtraValueVisitor : public boost::static_visitor<void> {
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
  static utils::SwappingSmart<Logger> default_logger_ptr(
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

}  // namespace

LoggerPtr DefaultLogger() { return DefaultLoggerInternal().Get(); }

LoggerPtr SetDefaultLogger(LoggerPtr logger) {
  // FIXME: we have to do atomic exchange() and return the old value.
  // SetDefaultLogger() is called only at startup, so mutex is OK here.
  // Better solution in https://st.yandex-team.ru/TAXICOMMON-234
  static std::mutex mutex;
  std::unique_lock<std::mutex> lock(mutex);

  auto& default_logger = DefaultLoggerInternal();
  auto old = default_logger.Get();
  default_logger.Set(logger);
  UpdateLogLevelCache();

  return old;
}

void SetDefaultLoggerLevel(Level level) {
  DefaultLogger()->set_level(static_cast<spdlog::level::level_enum>(level));
  UpdateLogLevelCache();
}

struct LogHelper::StreamImpl {
  MessageBuffer buffer_;
  std::ostream stream_;

  StreamImpl(Level level) : buffer_{level}, stream_(&buffer_) {}
};

std::ostream& LogHelper::Stream() noexcept { return pimpl_->stream_; }

void LogHelper::StartTskvEncoding() noexcept {
  return pimpl_->buffer_.StartTskvValueEncoding();
}
void LogHelper::EndTskvEncoding() noexcept {
  return pimpl_->buffer_.EndTskvValueEncoding();
}

LogHelper::LogHelper(Level level, const char* path, int line,
                     const char* func)
    : pimpl_(level)  // May throw bad_alloc
{
  NOTHROW_CALL_CONSTRUCTOR(path, line, LogSpan())
  NOTHROW_CALL_CONSTRUCTOR(path, line, LogModule(path, line, func))
  NOTHROW_CALL_CONSTRUCTOR(path, line, LogTaskIdAndCoroutineId())

  LogTextKey();  // This member outputs only a key without value
                 // This call must be the last in constructor
}

LogHelper::~LogHelper() { DoLog(); }

void LogHelper::DoLog() noexcept {
  NOTHROW_CALL_GENERIC(AppendLogExtra())
  NOTHROW_CALL_GENERIC(Stream().flush());

  try {
    static_cast<LoggerWorkaroud*>(DefaultLogger().get())
        ->sink_it_(pimpl_->buffer_.msg);
  } catch (...) {
    try {
      std::cerr << "LogHelper failed to sink_it_:"
                << boost::current_exception_diagnostic_information() << '\n';

      auto buf = static_cast<std::basic_streambuf<char>*>(&pimpl_->buffer_);
      NOTHROW_CALL_GENERIC(std::cerr << buf);
    } catch (...) {
    }
    assert(false && "LogHelper::DoLog()");
  }
}

void LogHelper::AppendLogExtra() {
  const auto& items = extra_.extra_;
  if (items->empty()) return;

  LogExtraValueVisitor visitor(*this);

  for (const auto& item : *items) {
    Stream() << utils::encoding::kTskvPairsSeparator;
    utils::encoding::EncodeTskv(
        Stream(), item.first,
        utils::encoding::EncodeTskvMode::kKeyReplacePeriod);
    Stream() << utils::encoding::kTskvKeyValueSeparator;
    boost::apply_visitor(visitor, item.second.GetValue());
  }
}

void LogHelper::LogTextKey() {
  Stream() << utils::encoding::kTskvPairsSeparator << "text"
           << utils::encoding::kTskvKeyValueSeparator;
}

void LogHelper::LogModule(const char* path, int line, const char* func) {
  Stream() << "module" << utils::encoding::kTskvKeyValueSeparator << func
           << " ( ";
  Stream() << path;
  Stream() << kPathLineSeparator << line << " ) ";
}

void LogHelper::LogTaskIdAndCoroutineId() {
  auto task = engine::current_task::GetCurrentTaskContextUnchecked();
  uint64_t task_id = task ? reinterpret_cast<uint64_t>(task) : 0;
  uint64_t coro_id = task ? task->GetCoroId() : 0;
  Stream() << utils::encoding::kTskvPairsSeparator << "task_id"
           << utils::encoding::kTskvKeyValueSeparator << task_id
           << utils::encoding::kTskvPairsSeparator << "coro_id"
           << utils::encoding::kTskvKeyValueSeparator << coro_id;
}

void LogHelper::LogSpan() {
  auto* span = tracing::Span::CurrentSpan();
  if (span) *this << *span;
}

void LogFlush() { DefaultLogger()->flush(); }

}  // namespace logging
