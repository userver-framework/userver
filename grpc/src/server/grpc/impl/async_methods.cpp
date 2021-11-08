#include <userver/server/grpc/impl/async_methods.hpp>

#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::grpc::impl {

void ReportErrorWhileCancelling(std::string_view call_name) noexcept {
  LOG_ERROR_TO(logging::DefaultLoggerOptional())
      << "Connection error while cancelling call '" << call_name << "'";
}

const ::grpc::Status kUnimplementedStatus{::grpc::StatusCode::UNIMPLEMENTED,
                                          "This method is unimplemented"};

}  // namespace server::grpc::impl

USERVER_NAMESPACE_END
