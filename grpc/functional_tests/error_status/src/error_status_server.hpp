#pragma once

#include <grpc/status.h>

#include <userver/components/component.hpp>
#include <userver/ugrpc/server/exceptions.hpp>
#include <userver/ugrpc/status_codes.hpp>
#include <userver/utest/using_namespace_userver.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <functional_tests/error_status_service.usrv.pb.hpp>

namespace functional_tests {

class ErrorStatusServiceComponent final : public api::ErrorStatusServiceBase::Component {
public:
    static constexpr std::string_view kName = "error-status-service";

    ErrorStatusServiceComponent(const components::ComponentConfig& config, const components::ComponentContext& context)
        : api::ErrorStatusServiceBase::Component(config, context) {}

    ReturnErrorStatusResult ReturnErrorStatus(CallContext& context, api::ErrorStatusRequest&& request) override;

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

    static yaml_config::Schema GetStaticConfigSchema();
};

}  // namespace functional_tests
