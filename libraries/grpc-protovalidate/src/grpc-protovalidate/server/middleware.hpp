#pragma once

#include <map>
#include <memory>
#include <string>

#include <userver/ugrpc/server/middlewares/base.hpp>
#include <userver/utils/impl/transparent_hash.hpp>

namespace buf::validate {

class ValidatorFactory;

}  // namespace buf::validate

USERVER_NAMESPACE_BEGIN

namespace grpc_protovalidate::server {

struct ValidationSettings {
    bool fail_fast{true};
    bool send_violations{false};
};

struct Settings final {
    ValidationSettings global;
    utils::impl::TransparentMap<std::string, ValidationSettings> per_method;

    const ValidationSettings& Get(std::string_view method_name) const;
};

class Middleware final : public ugrpc::server::MiddlewareBase {
public:
    explicit Middleware(const Settings& settings);
    ~Middleware() override;

    void PostRecvMessage(ugrpc::server::MiddlewareCallContext& context, google::protobuf::Message& request)
        const override;

private:
    Settings settings_;
    std::unique_ptr<buf::validate::ValidatorFactory> validator_factory_;
};

}  // namespace grpc_protovalidate::server

USERVER_NAMESPACE_END
