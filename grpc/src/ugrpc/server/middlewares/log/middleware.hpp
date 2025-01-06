#pragma once

#include <cstddef>
#include <optional>

#include <userver/logging/level.hpp>
#include <userver/ugrpc/server/middlewares/base.hpp>
#include <userver/ugrpc/server/storage_context.hpp>
#include <userver/utils/any_storage.hpp>
#include <userver/yaml_config/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::middlewares::log {

inline const utils::AnyStorageDataTag<StorageContext, bool> kIsFirstRequest;

struct Settings final {
    std::optional<logging::Level> local_log_level{};
    logging::Level msg_log_level{logging::Level::kDebug};
    std::size_t max_msg_size{512};
    bool trim_secrets{true};
};

Settings Parse(const yaml_config::YamlConfig& config, formats::parse::To<Settings>);

class Middleware final : public MiddlewareBase {
public:
    explicit Middleware(const Settings& settings);

    void Handle(MiddlewareCallContext& context) const override;

    void CallRequestHook(const MiddlewareCallContext& context, google::protobuf::Message& request) override;

    void CallResponseHook(const MiddlewareCallContext& context, google::protobuf::Message& response) override;

private:
    Settings settings_;
};

}  // namespace ugrpc::server::middlewares::log

USERVER_NAMESPACE_END
