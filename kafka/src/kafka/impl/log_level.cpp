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
  using enum logging::Level;
  using enum RdKafkaLogLevel;

  const auto rd_kafka_log_level = static_cast<RdKafkaLogLevel>(log_level);

  switch (rd_kafka_log_level) {
    case kRdLogEmerge:
    case kRdLogAlert:
    case kRdLogCrit:
      return kCritical;
    case kRdLogErr:
      return kError;
    case kRdLogWarning:
      return kWarning;
    case kRdLogNotice:
    case kRdLogInfo:
      return kInfo;
    case kRdLogDebug:
      return kDebug;
    default:
      return kInfo;
  }
}

}  // namespace kafka::impl

USERVER_NAMESPACE_END
