#include <grpc-protovalidate/server/middleware.hpp>

#include <utility>

#include <userver/grpc-protovalidate/validate.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace grpc_protovalidate::server {

namespace {

void LogError(const ValidationError& error) {
    switch (error.GetType()) {
        case ValidationError::Type::kInternal:
            LOG_ERROR() << error;
            return;
        case ValidationError::Type::kRule:
            LOG_WARNING() << error;
            return;
    }
    UINVARIANT(false, "Unexpected error type");
}

}  // namespace

const ValidationSettings& Settings::Get(std::string_view method_name) const {
    auto it = per_method.find(method_name);
    return it != per_method.end() ? it->second : global;
}

Middleware::Middleware(const Settings& settings) : settings_(settings) {}

Middleware::~Middleware() = default;

void Middleware::PostRecvMessage(ugrpc::server::MiddlewareCallContext& context, google::protobuf::Message& request)
    const {
    const ValidationSettings& settings = settings_.Get(context.GetCallName());
    const ValidationResult result = ValidateMessage(request, {.fail_fast = settings.fail_fast});
    if (result.IsSuccess()) {
        return;
    }
    LogError(result.GetError());
    context.SetError(result.GetError().GetGrpcStatus(settings.send_violations));
}

}  // namespace grpc_protovalidate::server

USERVER_NAMESPACE_END
