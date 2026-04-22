#pragma once

#include <grpc/status.h>

#include <userver/components/component.hpp>
#include <userver/ugrpc/server/exceptions.hpp>
#include <userver/ugrpc/status_codes.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <functional_tests/error_status_service.usrv.pb.hpp>

#include <userver/utest/using_namespace_userver.hpp>

namespace functional_tests {

class ErrorStatusServiceComponent final : public api::ErrorStatusServiceBase::Component {
public:
    static constexpr std::string_view kName = "error-status-service";

    ErrorStatusServiceComponent(const components::ComponentConfig& config, const components::ComponentContext& context)
        : api::ErrorStatusServiceBase::Component(config, context) {}

    ReturnErrorStatusResult ReturnErrorStatus(CallContext& context, api::ErrorStatusRequest&& request) override;

    /// @warning Returning non-standard gRPC status codes violates the gRPC spec
    /// and may lead to undefined behavior when values are outside the valid enum range.
    ///
    /// This method exists solely for testing error handling pathways and non-standard code
    /// should never be used in production code. The gRPC specification strictly defines valid status
    /// codes in the range [0, 16].
    ///
    /// @see https://github.com/grpc/grpc/blob/master/doc/statuscodes.md?plain=1#L28
    /// @see https://nda.ya.ru/t/m2doGh4e7Nipk4
    ReturnErrorStatusResult ReturnNonStandardErrorStatus(CallContext& context, api::ErrorStatusRequest&& request)
        override;

    ThrowErrorWithStatusExceptionResult ThrowErrorWithStatusException(
        CallContext& context,
        api::ErrorStatusRequest&& request
    ) override;

    ThrowRuntimeErrorExceptionResult ThrowRuntimeErrorException(
        CallContext& context,
        api::RuntimeErrorRequest&& request
    ) override;

    ReturnStreamErrorStatusResult ReturnStreamErrorStatus(
        CallContext& context,
        api::ErrorStatusRequest&& request,
        ReturnStreamErrorStatusWriter& writer
    ) override;

    ThrowStreamErrorWithStatusExceptionResult ThrowStreamErrorWithStatusException(
        CallContext& context,
        api::ErrorStatusRequest&& request,
        ThrowStreamErrorWithStatusExceptionWriter& writer
    ) override;

    ReturnStreamNonStandardErrorStatusResult ReturnStreamNonStandardErrorStatus(
        CallContext& context,
        api::ErrorStatusRequest&& request,
        ReturnStreamNonStandardErrorStatusWriter& writer
    ) override;

    SupportsNonStandardStatusResult SupportsNonStandardStatus(
        CallContext& context,
        api::NonStandardErrorStatusSupportRequest&& request
    ) override;

    static yaml_config::Schema GetStaticConfigSchema();
};

}  // namespace functional_tests
