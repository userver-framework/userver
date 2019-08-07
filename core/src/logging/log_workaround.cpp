#include <logging/log_workaround.hpp>

// Forgive me Father for I have sinned -- TAXICOMMON-1319
#undef SPDLOG_FINAL
#define SPDLOG_FINAL
#include <spdlog/async_logger.h>

namespace logging {
namespace impl {
namespace {

// exposes protected logger method to pass preformatted messages
class SyncLoggerWorkaroud final : public spdlog::logger {
 public:
  using spdlog::logger::sink_it_;
};

// exposes protected async_logger method to pass preformatted messages
class AsyncLoggerWorkaround final : public spdlog::async_logger {
 public:
  using spdlog::async_logger::sink_it_;
};

}  // namespace

void SinkMessage(const logging::LoggerPtr& logger_ptr,
                 spdlog::details::log_msg& message) {
  auto* logger = logger_ptr.get();
  if (auto* async_logger = dynamic_cast<spdlog::async_logger*>(logger)) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
    static_cast<AsyncLoggerWorkaround*>(async_logger)->sink_it_(message);
  } else {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
    static_cast<SyncLoggerWorkaroud*>(logger)->sink_it_(message);
  }
}

}  // namespace impl
}  // namespace logging
