#include <userver/ugrpc/server/impl/service_worker_impl.hpp>

#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

void ReportHandlerError(const std::exception& ex,
                        std::string_view call_name) noexcept {
  LOG_ERROR_TO(logging::DefaultLoggerOptional())
      << "Uncaught exception in '" << call_name << "'. " << ex;
}

void ReportNonStdHandlerError(std::string_view call_name) noexcept {
  LOG_ERROR_TO(logging::DefaultLoggerOptional())
      << "Uncaught exception of in '" << call_name
      << "'. The exception type is not derived from 'std::exception'. Please "
         "avoid throwing non-std::exception-derived types.";
}

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
