#include <error_status_server.hpp>

namespace functional_tests {

static constexpr int kCountSend = 3;

ErrorStatusServiceComponent::ReturnErrorStatusResult
ErrorStatusServiceComponent::ReturnErrorStatus(CallContext&, api::ErrorStatusRequest&& request) {
    const grpc::StatusCode request_status_code = ugrpc::StatusCodeFromString(request.status_code());
    if (request_status_code == grpc::StatusCode::OK) {
        return api::ErrorStatusResponse{};
    }
    return grpc::Status{
        request_status_code,
        request.status_message(),
        "",
    };
}

ErrorStatusServiceComponent::ThrowErrorWithStatusExceptionResult
ErrorStatusServiceComponent::ThrowErrorWithStatusException(CallContext&, api::ErrorStatusRequest&& request) {
    const grpc::StatusCode request_status_code = ugrpc::StatusCodeFromString(request.status_code());
    if (request_status_code == grpc::StatusCode::OK) {
        return api::ErrorStatusResponse{};
    }
    throw ugrpc::server::ErrorWithStatus{
        request_status_code,
        request.status_message(),
    };
}

ErrorStatusServiceComponent::ThrowRuntimeErrorExceptionResult
ErrorStatusServiceComponent::ThrowRuntimeErrorException(CallContext&, api::RuntimeErrorRequest&& request) {
    throw std::runtime_error(request.message());
}

ErrorStatusServiceComponent::ReturnStreamErrorStatusResult ErrorStatusServiceComponent::ReturnStreamErrorStatus(
    CallContext&,
    api::ErrorStatusRequest&& request,
    ReturnStreamErrorStatusWriter& writer
) {
    api::ErrorStatusResponse response;
    for (int i = 0; i < kCountSend; ++i) {
        writer.Write(response);
    }

    const grpc::StatusCode request_status_code = ugrpc::StatusCodeFromString(request.status_code());
    if (request_status_code == grpc::StatusCode::OK) {
        return grpc::Status::OK;
    }

    return grpc::Status{
        request_status_code,
        request.status_message(),
        "",
    };
}

ErrorStatusServiceComponent::ThrowStreamErrorWithStatusExceptionResult
ErrorStatusServiceComponent::ThrowStreamErrorWithStatusException(
    CallContext&,
    api::ErrorStatusRequest&& request,
    ThrowStreamErrorWithStatusExceptionWriter& writer
) {
    api::ErrorStatusResponse response;
    for (int i = 0; i < kCountSend; ++i) {
        writer.Write(response);
    }

    const grpc::StatusCode request_status_code = ugrpc::StatusCodeFromString(request.status_code());
    if (request_status_code == grpc::StatusCode::OK) {
        return grpc::Status::OK;
    }

    throw ugrpc::server::ErrorWithStatus{
        request_status_code,
        request.status_message(),
    };
}

yaml_config::Schema ErrorStatusServiceComponent::GetStaticConfigSchema() {
    return yaml_config::MergeSchemas<ugrpc::server::ServiceComponentBase>(R"(
type: object
description: gRPC error status service component for testing error handling and status codes
additionalProperties: false
properties: {}
)");
}

}  // namespace functional_tests
