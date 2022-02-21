#include <userver/ugrpc/server/impl/service_worker_impl.hpp>

#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

void ReportHandlerError(const std::exception& ex,
                        std::string_view call_name) noexcept {
  LOG_ERROR_TO(logging::DefaultLoggerOptional())
      << "Uncaught exception in '" << call_name << "': " << ex;
}

void ReportNetworkError(const RpcInterruptedError& ex,
                        std::string_view call_name) noexcept {
  LOG_WARNING_TO(logging::DefaultLoggerOptional())
      << "Network error in '" << call_name << "': " << ex;
}

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
