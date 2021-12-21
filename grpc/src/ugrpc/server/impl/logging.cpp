#include <ugrpc/server/impl/logging.hpp>

#include <stdexcept>

#include <fmt/format.h>
#include <grpc/impl/codegen/log.h>

#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

namespace {

logging::Level ToLogLevel(::gpr_log_severity severity) noexcept {
  switch (severity) {
    case ::GPR_LOG_SEVERITY_DEBUG:
      return logging::Level::kDebug;
    case ::GPR_LOG_SEVERITY_INFO:
      return logging::Level::kInfo;
    case ::GPR_LOG_SEVERITY_ERROR:
      [[fallthrough]];
    default:
      return logging::Level::kError;
  }
}

::gpr_log_severity ToGprLogSeverity(logging::Level level) {
  switch (level) {
    case logging::Level::kDebug:
      return ::GPR_LOG_SEVERITY_DEBUG;
    case logging::Level::kInfo:
      return ::GPR_LOG_SEVERITY_INFO;
    case logging::Level::kError:
      return ::GPR_LOG_SEVERITY_ERROR;
    default:
      throw std::logic_error(
          fmt::format("grpcpp log level {} is not allowed. Allowed options: "
                      "debug, info, error.",
                      logging::ToString(level)));
  }
}

void LogFunction(::gpr_log_func_args* args) noexcept {
  UASSERT(args);
  const auto lvl = ToLogLevel(args->severity);
  if (!logging::ShouldLog(lvl)) return;

  logging::LogHelper(logging::DefaultLoggerOptional(), lvl, args->file,
                     args->line, "")
      << args->message;
}

}  // namespace

void SetupLogging(logging::Level min_log_level_override) {
  ::gpr_set_log_function(&LogFunction);
  ::gpr_set_log_verbosity(ToGprLogSeverity(min_log_level_override));
}

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
