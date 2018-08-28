#include <logging/log.hpp>

#include <atomic>

#include <boost/lexical_cast.hpp>

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

LogHelper::LogHelper(Level level, const char* path, int line, const char* func)
    : log_msg_(&DefaultLogger()->name(),
               static_cast<spdlog::level::level_enum>(level)) {
  LogModule(path, line, func);
  LogTextKey();
}

void LogHelper::DoLog() {
  AppendLogExtra();
  DefaultLogger()->_sink_it(log_msg_);
}

void LogHelper::AppendLogExtra() {
  const auto& items = extra_.extra_;
  if (items.empty()) return;

  LogExtraValueVisitor visitor(*this);

  for (const auto& item : items) {
    log_msg_.raw << utils::encoding::kTskvPairsSeparator;
    utils::encoding::EncodeTskv(log_msg_.raw, item.first,
                                utils::encoding::EncodeTskvMode::kKey);
    log_msg_.raw << utils::encoding::kTskvKeyValueSeparator;
    boost::apply_visitor(visitor, item.second.GetValue());
  }
}

void LogHelper::LogTextKey() {
  log_msg_.raw << utils::encoding::kTskvPairsSeparator << "text"
               << utils::encoding::kTskvKeyValueSeparator;
}

void LogHelper::LogModule(const char* path, int line, const char* func) {
  log_msg_.raw << "module" << utils::encoding::kTskvKeyValueSeparator << func
               << " ( ";
  *this << path;
  log_msg_.raw << kPathLineSeparator << line << " ) ";
}

LogHelper& operator<<(LogHelper& lh, std::thread::id id) {
  lh << boost::lexical_cast<std::string>(id);
  return lh;
}

}  // namespace logging
