#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utest/using_namespace_userver.hpp>  // IWYU pragma: keep

#include <chrono>
#include <string>
#include <string_view>
#include <vector>

#include <fmt/format.h>

#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component_list.hpp>
#include <userver/components/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/storages/redis/client.hpp>
#include <userver/storages/redis/component.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/storages/secdist/provider_component.hpp>
#include <userver/utils/daemon_run.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

namespace chaos {

class KeyValue final : public server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-redis";

    KeyValue(const components::ComponentConfig& config, const components::ComponentContext& context);

    std::string HandleRequestThrow(const server::http::HttpRequest& request, server::request::RequestContext&)
        const override;

    static yaml_config::Schema GetStaticConfigSchema();

private:
    std::string GetValue(std::string_view key, const server::http::HttpRequest& request) const;
    std::string PostValue(std::string_view key, const server::http::HttpRequest& request) const;
    std::string PostBlpop(std::string_view key, std::string_view blpop_wait_sec) const;
    std::string DeleteValue(std::string_view key) const;

    storages::redis::ClientPtr redis_client_;
    storages::redis::CommandControl redis_cc_;
};

KeyValue::KeyValue(const components::ComponentConfig& config, const components::ComponentContext& context)
    : server::handlers::HttpHandlerBase(config, context),
      redis_client_{
          context.FindComponent<components::Redis>("key-value-database").GetClient(config["db"].As<std::string>())
      }
{}

std::string KeyValue::HandleRequestThrow(
    const server::http::HttpRequest& request,
    server::request::RequestContext& /*context*/
) const {
    const auto& key = request.GetArg("key");
    if (key.empty()) {
        throw server::handlers::ClientError(server::handlers::ExternalBody{"No 'key' query argument"});
    }

    switch (request.GetMethod()) {
        case server::http::HttpMethod::kGet:
            return GetValue(key, request);
        case server::http::HttpMethod::kPost: {
            const auto& blpop_wait_sec = request.GetArg("blpop_wait_sec");
            if (!blpop_wait_sec.empty()) {
                return PostBlpop(key, blpop_wait_sec);
            }
            return PostValue(key, request);
        }
        case server::http::HttpMethod::kDelete:
            return DeleteValue(key);
        default:
            throw server::handlers::ClientError(server::handlers::ExternalBody{
                fmt::format("Unsupported method {}", request.GetMethod())
            });
    }
}

yaml_config::Schema KeyValue::GetStaticConfigSchema() {
    return yaml_config::MergeSchemas<HandlerBase>(R"(
type: object
description: KeyValue handler schema
additionalProperties: false
properties:
    db:
        type: string
        description: redis database name
)");
}

std::string KeyValue::GetValue(std::string_view key, const server::http::HttpRequest& request) const {
    const auto result = redis_client_->Get(std::string{key}, redis_cc_).Get();
    if (!result) {
        request.SetResponseStatus(server::http::HttpStatus::kNotFound);
        return {};
    }
    return *result;
}

std::string KeyValue::PostBlpop(std::string_view key, std::string_view blpop_wait_sec) const {
    constexpr size_t kKeyIndex = 0;
    const storages::redis::CommandControl blpop_cc{std::chrono::seconds{120}, std::chrono::seconds{120}, 4};
    using BlpopReply = std::vector<std::string>;
    const auto result =
        redis_client_
            ->GenericCommand<BlpopReply>("BLPOP", {std::string{key}, std::string{blpop_wait_sec}}, kKeyIndex, blpop_cc)
            .Get();
    if (result.size() >= 2) {
        return fmt::format("{} {}", result[0], result[1]);
    }
    return std::string{};
}

std::string KeyValue::PostValue(std::string_view key, const server::http::HttpRequest& request) const {
    const auto& value = request.GetArg("value");
    const auto result = redis_client_->SetIfNotExist(std::string{key}, value, redis_cc_).Get();
    if (!result) {
        request.SetResponseStatus(server::http::HttpStatus::kConflict);
        return {};
    }

    request.SetResponseStatus(server::http::HttpStatus::kCreated);
    return std::string{value};
}

std::string KeyValue::DeleteValue(std::string_view key) const {
    const auto result = redis_client_->Del(std::string{key}, redis_cc_).Get();
    return std::to_string(result);
}

}  // namespace chaos

int main(int argc, char* argv[]) {
    const auto component_list =
        components::MinimalServerComponentList()
            .Append<chaos::KeyValue>("handler-cluster")
            .Append<chaos::KeyValue>("handler-sentinel")
            .Append<chaos::KeyValue>("handler-standalone")
            .AppendComponentList(clients::http::ComponentList())
            .Append<components::Secdist>()
            .Append<components::DefaultSecdistProvider>()
            .Append<components::Redis>("key-value-database")
            .Append<components::TestsuiteSupport>()
            .Append<server::handlers::TestsControl>()
            .Append<clients::dns::Component>();
    return utils::DaemonMain(argc, argv, component_list);
}
