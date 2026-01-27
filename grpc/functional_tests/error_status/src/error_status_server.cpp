#include <error_status_server.hpp>

#include <userver/utils/from_string.hpp>

namespace functional_tests {

static constexpr int kCountSend = 3;

ErrorStatusServiceComponent::ReturnErrorStatusResult ErrorStatusServiceComponent::ReturnErrorStatus(
    CallContext&,
    api::ErrorStatusRequest&& request
) {
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

ErrorStatusServiceComponent::ReturnErrorStatusResult ErrorStatusServiceComponent::ReturnNonStandardErrorStatus(
    CallContext&,
    api::ErrorStatusRequest&& request
) {
    /// @warning Direct integer-to-enum cast may cause UB for invalid values
    /// Use @ref ugrpc::StatusCodeFromString for safe conversion
    /// This cast exists solely to test gRPC error handling with arbitrary status codes
    const int request_status_code_int = utils::FromString<int>(request.status_code());
    const auto request_status_code = static_cast<grpc::StatusCode>(request_status_code_int);
    if (request_status_code_int != request_status_code) {
        throw std::runtime_error("Status code enum overflow. Refactor the test");
    }
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

ErrorStatusServiceComponent::ThrowRuntimeErrorExceptionResult ErrorStatusServiceComponent::ThrowRuntimeErrorException(
    CallContext&,
    api::RuntimeErrorRequest&& request
) {
    throw std::runtime_error(request.message());
}

ErrorStatusServiceComponent::ReturnStreamErrorStatusResult ErrorStatusServiceComponent::ReturnStreamErrorStatus(
    CallContext&,
    api::ErrorStatusRequest&& request,
    ReturnStreamErrorStatusWriter& writer
) {
    for (int i = 0; i < kCountSend; ++i) {
        writer.Write(api::ErrorStatusResponse{});
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
    for (int i = 0; i < kCountSend; ++i) {
        writer.Write(api::ErrorStatusResponse{});
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

ErrorStatusServiceComponent::ReturnStreamNonStandardErrorStatusResult
ErrorStatusServiceComponent::ReturnStreamNonStandardErrorStatus(
    CallContext&,
    api::ErrorStatusRequest&& request,
    ReturnStreamNonStandardErrorStatusWriter& writer
) {
    for (int i = 0; i < kCountSend; ++i) {
        auto response = api::ErrorStatusResponse{};
        response.set_non_empty_msg(true);
        writer.Write(std::move(response));
    }

    /// @warning Direct integer-to-enum cast may cause UB for invalid values
    /// Use @ref ugrpc::StatusCodeFromString for safe conversion
    /// This cast exists solely to test gRPC error handling with arbitrary status codes
    const int request_status_code_int = std::stoi(request.status_code());
    const auto request_status_code = static_cast<grpc::StatusCode>(request_status_code_int);
    if (request_status_code_int != request_status_code) {
        throw std::runtime_error("Status code enum overflow. Refactor the test");
    }
    if (request_status_code == grpc::StatusCode::OK) {
        return api::ErrorStatusResponse{};
    }
    return grpc::Status{
        request_status_code,
        request.status_message(),
        "",
    };
}

ErrorStatusServiceComponent::SupportsNonStandardStatusResult
ErrorStatusServiceComponent::SupportsNonStandardStatus(CallContext&, api::NonStandardErrorStatusSupportRequest&&) {
    auto result = api::NonStandardErrorStatusSupportResponse();
    result.set_supports_non_standard_statuses(true);

#if defined(__has_feature)
#if __has_feature(undefined_behavior_sanitizer)
    result.set_supports_non_standard_statuses(false);
#endif
#endif

    return result;
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
