#include <grpc-protovalidate/client/middleware.hpp>

#include <google/protobuf/arena.h>

#include <userver/grpc-protovalidate/client/exceptions.hpp>
#include <userver/logging/log.hpp>

#include <grpc-protovalidate/impl/utils.hpp>

USERVER_NAMESPACE_BEGIN

namespace grpc_protovalidate::client {

const ValidationSettings& Settings::Get(std::string_view method_name) const {
    auto it = per_method.find(method_name);
    return it != per_method.end() ? it->second : global;
}

Middleware::Middleware(const Settings& settings)
    : settings_(settings), validator_factory_(grpc_protovalidate::impl::CreateProtoValidatorFactory()) {}

Middleware::~Middleware() = default;

void Middleware::PostRecvMessage(
    ugrpc::client::MiddlewareCallContext& context,
    const google::protobuf::Message& message
) const {
    const auto& settings = settings_.Get(context.GetCallName());
    google::protobuf::Arena arena;

    auto validator = validator_factory_->NewValidator(&arena, settings.fail_fast);
    auto result = validator.Validate(message);

    if (!result.ok()) {
        // Usually that means some CEL expression in a proto file can not be
        // parsed or evaluated.
        LOG_ERROR() << "Validator internal error (check response constraints in the proto file): " << result.status();
        throw ValidatorError(context.GetCallName());
    } else if (result.value().violations_size() > 0) {
        for (const auto& violation : result.value().violations()) {
            LOG_ERROR() << "Response constraint violation: " << violation.proto();
        }

        throw ResponseError(context.GetCallName(), std::move(result).value());
    } else {
        LOG_DEBUG() << "Response is valid";
    }
}

}  // namespace grpc_protovalidate::client

USERVER_NAMESPACE_END
