#include <grpc-protovalidate/server/middleware.hpp>

#include <fmt/core.h>
#include <google/protobuf/arena.h>

#include <userver/logging/log.hpp>
#include <userver/ugrpc/status_utils.hpp>

#include <grpc-protovalidate/impl/utils.hpp>

USERVER_NAMESPACE_BEGIN

namespace grpc_protovalidate::server {

const ValidationSettings& Settings::Get(std::string_view method_name) const {
    auto it = per_method.find(method_name);
    return it != per_method.end() ? it->second : global;
}

Middleware::Middleware(const Settings& settings)
    : settings_(settings), validator_factory_(grpc_protovalidate::impl::CreateProtoValidatorFactory()) {}

Middleware::~Middleware() = default;

void Middleware::PostRecvMessage(ugrpc::server::MiddlewareCallContext& context, google::protobuf::Message& request)
    const {
    const auto& settings = settings_.Get(context.GetCallName());
    google::protobuf::Arena arena;

    auto validator = validator_factory_->NewValidator(&arena, settings.fail_fast);
    auto result = validator.Validate(request);

    if (!result.ok()) {
        // Usually that means some CEL expression in a proto file can not be parsed or evaluated.
        context.SetError(grpc::Status{
            grpc::StatusCode::INTERNAL,
            fmt::format(
                "Validator internal error (check request constraints in the proto file): {}", result.status().ToString()
            )});
    } else if (result.value().violations_size() > 0) {
        for (const auto& violation : result.value().violations()) {
            LOG_ERROR() << "Request constraint violation: " << violation.proto();
        }

        if (settings.send_violations) {
            google::rpc::Status gstatus;
            gstatus.set_code(grpc::StatusCode::INVALID_ARGUMENT);
            gstatus.set_message(fmt::format("Request violates constraints (count={})", result.value().violations_size())
            );
            gstatus.add_details()->PackFrom(result.value().proto());
            context.SetError(ugrpc::ToGrpcStatus(gstatus));
        } else {
            context.SetError(grpc::Status{
                grpc::StatusCode::INVALID_ARGUMENT,
                fmt::format("Request violates constraints (count={})", result.value().violations_size())});
        }
    } else {
        LOG_DEBUG() << "Request is valid";
    }
}

}  // namespace grpc_protovalidate::server

USERVER_NAMESPACE_END
