#include <grpc-protovalidate/client/middleware.hpp>

#include <google/protobuf/arena.h>

#include <userver/grpc-protovalidate/client/exceptions.hpp>
#include <userver/grpc-protovalidate/validate.hpp>
#include <userver/logging/log.hpp>
#include <userver/ugrpc/client/exceptions.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace grpc_protovalidate::client {

const ValidationSettings& Settings::Get(std::string_view method_name) const {
    auto it = per_method.find(method_name);
    return it != per_method.end() ? it->second : global;
}

Middleware::Middleware(const Settings& settings)
    : settings_(settings)
{}

Middleware::~Middleware() = default;

void Middleware::PreSendMessage(ugrpc::client::MiddlewareCallContext& context, const google::protobuf::Message& request)
    const {
    const ValidationSettings& settings = settings_.Get(context.GetCallName());
    if (!settings.validate_requests) {
        return;
    }
    const ValidationResult result = ValidateMessage(request, {.fail_fast = settings.fail_fast});
    if (result.IsSuccess()) {
        return;
    }
    const ValidationError& error = result.GetError();
    switch (error.GetType()) {
        case ValidationError::Type::kInternal:
            throw ValidatorError(context.GetCallName());
        case ValidationError::Type::kRule:
            LOG_WARNING() << error;
            throw RequestError(context.GetCallName(), error.GetViolations());
    }
    UINVARIANT(false, "Unexpected error type");
}

void Middleware::PostRecvMessage(
    ugrpc::client::MiddlewareCallContext& context,
    const google::protobuf::Message& response
) const {
    const ValidationSettings& settings = settings_.Get(context.GetCallName());
    const ValidationResult result = ValidateMessage(response, {.fail_fast = settings.fail_fast});
    if (result.IsSuccess()) {
        return;
    }
    const ValidationError& error = result.GetError();
    switch (error.GetType()) {
        case ValidationError::Type::kInternal:
            throw ValidatorError(context.GetCallName());
        case ValidationError::Type::kRule:
            LOG_WARNING() << error;
            throw ResponseError(context.GetCallName(), error.GetViolations());
    }
    UINVARIANT(false, "Unexpected error type");
}

}  // namespace grpc_protovalidate::client

USERVER_NAMESPACE_END
