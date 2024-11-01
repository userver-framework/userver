#pragma once

#include <string>

#include <userver/ugrpc/server/middlewares/base.hpp>

namespace google::protobuf {

class Message;

}  // namespace google::protobuf

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::middlewares::field_mask {

class Middleware final : public MiddlewareBase {
public:
    explicit Middleware(std::string metadata_field_name);

    void Handle(MiddlewareCallContext& context) const override;

    void CallResponseHook(const ugrpc::server::MiddlewareCallContext& context, google::protobuf::Message& response)
        override;

private:
    const std::string metadata_field_name_;
};

}  // namespace ugrpc::server::middlewares::field_mask

USERVER_NAMESPACE_END
