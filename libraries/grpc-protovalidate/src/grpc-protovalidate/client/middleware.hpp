#pragma once

#include <map>
#include <memory>
#include <string>

#include <userver/ugrpc/client/middlewares/base.hpp>
#include <userver/utils/impl/transparent_hash.hpp>

namespace buf::validate {

class ValidatorFactory;

}  // namespace buf::validate

USERVER_NAMESPACE_BEGIN

namespace grpc_protovalidate::client {

struct ValidationSettings {
    bool fail_fast{true};
};

struct Settings final {
    ValidationSettings global;
    utils::impl::TransparentMap<std::string, ValidationSettings> per_method;

    const ValidationSettings& Get(std::string_view method_name) const;
};

class Middleware final : public ugrpc::client::MiddlewareBase {
public:
    explicit Middleware(const Settings& settings);
    ~Middleware() override;

    void PostRecvMessage(ugrpc::client::MiddlewareCallContext& context, const google::protobuf::Message& message)
        const override;

private:
    Settings settings_;
    std::unique_ptr<buf::validate::ValidatorFactory> validator_factory_;
};

}  // namespace grpc_protovalidate::client

USERVER_NAMESPACE_END
