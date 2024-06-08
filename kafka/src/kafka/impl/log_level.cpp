#include <kafka/impl/log_level.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka::impl {

namespace {

/// @note `librdkafka` use the same log level as `syslog`
/// @see
/// https://github.com/confluentinc/librdkafka/blob/master/src/rd.h#L80
enum class RdKafkaLogLevel {
  kRdLogEmerge = 0,
  kRdLogAlert = 1,
  kRdLogCrit = 2,
  kRdLogErr = 3,
  kRdLogWarning = 4,
  kRdLogNotice = 5,
  kRdLogInfo = 6,
  kRdLogDebug = 7,
};

}  // namespace

logging::Level convertRdKafkaLogLevelToLoggingLevel(int log_level) noexcept {
  const auto rd_kafka_log_level = static_cast<RdKafkaLogLevel>(log_level);

  switch (rd_kafka_log_level) {
    case RdKafkaLogLevel::kRdLogEmerge:
    case RdKafkaLogLevel::kRdLogAlert:
    case RdKafkaLogLevel::kRdLogCrit:
      return logging::Level::kCritical;
    case RdKafkaLogLevel::kRdLogErr:
      return logging::Level::kError;
    case RdKafkaLogLevel::kRdLogWarning:
      return logging::Level::kWarning;
    case RdKafkaLogLevel::kRdLogNotice:
    case RdKafkaLogLevel::kRdLogInfo:
      return logging::Level::kInfo;
    case RdKafkaLogLevel::kRdLogDebug:
      return logging::Level::kDebug;
    default:
      return logging::Level::kInfo;
  }
}

}  // namespace kafka::impl

USERVER_NAMESPACE_END
