#include <logging/log.hpp>

#include <atomic>

#include <boost/lexical_cast.hpp>

#include <engine/task/task_context.hpp>
#include <logging/spdlog.hpp>
#include "log_streambuf.hpp"
#include "log_workaround.hpp"
#include "tskv_stream.hpp"

namespace logging {
namespace {

const char kPathLineSeparator = ':';

class LogExtraValueVisitor : public boost::static_visitor<void> {
 public:
  LogExtraValueVisitor(LogHelper& lh) : lh_(lh) {}

  template <typename T>
  void operator()(const T& value) const {
    lh_ << value;
  }

 private:
  LogHelper& lh_;
};

LoggerPtr* DefaultLoggerInternal() {
  static LoggerPtr default_logger = MakeStderrLogger("default");
  return &default_logger;
}

}  // namespace

LoggerPtr DefaultLogger() {
  return std::atomic_load_explicit(DefaultLoggerInternal(),
                                   std::memory_order_acquire);
}

LoggerPtr SetDefaultLogger(LoggerPtr logger) {
  return std::atomic_exchange_explicit(
      DefaultLoggerInternal(), std::move(logger), std::memory_order_acq_rel);
}

void SetDefaultLoggerLevel(Level level) {
  DefaultLogger()->set_level(static_cast<spdlog::level::level_enum>(level));
}

LogHelper::LogHelper(Level level, const char* path, int line, const char* func)
    : buffer_{std::make_unique<MessageBuffer>(level)},
      verbatim_stream_(buffer_.get()),
      tskv_buffer_{std::make_unique<TskvBuffer>(buffer_->msg.raw)},
      tskv_stream_(tskv_buffer_.get()) {
  LogModule(path, line, func);
  LogTaskIdAndCoroutineId();
  LogTextKey();  // This member outputs only a key without value
                 // This call must be the last in constructor
}

LogHelper::~LogHelper() noexcept(false) { DoLog(); }

void LogHelper::DoLog() {
  AppendLogExtra();
  verbatim_stream_.flush();
  static_cast<LoggerWorkaroud*>(DefaultLogger().get())->sink_it_(buffer_->msg);
}

void LogHelper::AppendLogExtra() {
  const auto& items = extra_.extra_;
  if (items.empty()) return;

  LogExtraValueVisitor visitor(*this);

  for (const auto& item : items) {
    verbatim_stream_ << utils::encoding::kTskvPairsSeparator;
    utils::encoding::EncodeTskv(verbatim_stream_, item.first,
                                utils::encoding::EncodeTskvMode::kKey);
    verbatim_stream_ << utils::encoding::kTskvKeyValueSeparator;
    boost::apply_visitor(visitor, item.second.GetValue());
  }
}

void LogHelper::LogTextKey() {
  verbatim_stream_ << utils::encoding::kTskvPairsSeparator << "text"
                   << utils::encoding::kTskvKeyValueSeparator;
}

void LogHelper::LogModule(const char* path, int line, const char* func) {
  verbatim_stream_ << "module" << utils::encoding::kTskvKeyValueSeparator
                   << func << " ( ";
  verbatim_stream_ << path;
  verbatim_stream_ << kPathLineSeparator << line << " ) ";
}

void LogHelper::LogTaskIdAndCoroutineId() {
  auto task = engine::current_task::GetCurrentTaskContextUnchecked();
  uint64_t task_id = task ? reinterpret_cast<uint64_t>(task) : 0;
  uint64_t coro_id = task ? task->GetCoroId() : 0;
  verbatim_stream_ << utils::encoding::kTskvPairsSeparator << "task_id"
                   << utils::encoding::kTskvKeyValueSeparator << task_id
                   << utils::encoding::kTskvPairsSeparator << "coro_id"
                   << utils::encoding::kTskvKeyValueSeparator << coro_id;
}

void LogFlush() { DefaultLogger()->flush(); }

}  // namespace logging
