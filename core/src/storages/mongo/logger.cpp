#include "logger.hpp"

#include <stdexcept>

#include <logging/level.hpp>
#include <logging/log.hpp>

namespace storages {
namespace mongo {
namespace impl {
namespace {

logging::Level ConvertLogLevel(mongocxx::log_level level) {
  switch (level) {
    case mongocxx::log_level::k_error:
    case mongocxx::log_level::k_critical:
    case mongocxx::log_level::k_warning:
      return logging::Level::kWarning;

    case mongocxx::log_level::k_message:
    case mongocxx::log_level::k_info:
      return logging::Level::kDebug;

    case mongocxx::log_level::k_debug:
    case mongocxx::log_level::k_trace:
      return logging::Level::kTrace;
  }
  LOG_WARNING() << "Unexpected mongocxx log level";
  return logging::Level::kWarning;
}

}  // namespace

void Logger::operator()(mongocxx::log_level level,
                        mongocxx::stdx::string_view domain,
                        mongocxx::stdx::string_view message) noexcept {
  try {
    LOG(ConvertLogLevel(level))
        << "Mongo driver " << to_string(level).to_string() << " ["
        << domain.to_string() << "]: " << message.to_string();
  } catch (const std::exception&) {
    // cannot throw here
  }
}

}  // namespace impl
}  // namespace mongo
}  // namespace storages
