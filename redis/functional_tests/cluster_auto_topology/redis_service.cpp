#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utest/using_namespace_userver.hpp>  // IWYU pragma: keep

#include <iostream>
#include <string>
#include <string_view>

#include <fmt/format.h>

#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component_list.hpp>
#include <userver/components/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/dynamic_config/updater/component_list.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/handlers/ping.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/storages/redis/client.hpp>
#include <userver/storages/redis/component.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/storages/secdist/provider_component.hpp>
#include <userver/utils/daemon_run.hpp>
#include <userver/yaml_config/merge_schemas.hpp>
#include "userver/storages/redis/exception.hpp"

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
        case server::http::HttpMethod::kPost:
            return PostValue(key, request);
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
    try {
        const auto result = redis_client_->Get(std::string{key}, redis_cc_).Get();
        if (!result) {
            request.SetResponseStatus(server::http::HttpStatus::kNotFound);
            return {};
        }
        return *result;
    } catch (const storages::redis::RequestFailedException& e) {
        std::cout << "!!! EXCEPTION GET: " << e.what() << ", " << e.GetStatusString() << "\n";
        throw;
    }
}

std::string KeyValue::PostValue(std::string_view key, const server::http::HttpRequest& request) const {
    try {
        const auto& value = request.GetArg("value");
        const auto result = redis_client_->SetIfNotExist(std::string{key}, value, redis_cc_).Get();
        if (!result) {
            request.SetResponseStatus(server::http::HttpStatus::kConflict);
            return {};
        }

        request.SetResponseStatus(server::http::HttpStatus::kCreated);
        return std::string{value};
    } catch (const storages::redis::RequestFailedException& e) {
        std::cout << "!!! EXCEPTION POST: " << e.what() << ", " << e.GetStatusString() << "\n";
        throw;
    }
}

std::string KeyValue::DeleteValue(std::string_view key) const {
    const auto result = redis_client_->Del(std::string{key}, redis_cc_).Get();
    return std::to_string(result);
}

class IsReady final : public server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-is-ready";

    IsReady(const components::ComponentConfig& config, const components::ComponentContext& context)
        : server::handlers::HttpHandlerBase(config, context),
          redis_component_(context.FindComponent<components::Redis>("key-value-database"))
    {}

    std::string HandleRequestThrow(const server::http::HttpRequest& request, server::request::RequestContext&)
        const override {
        const auto& db_name = request.GetArg("db");
        const auto& mode_str = request.GetArg("mode");
        const auto& max_failed_shards_str = request.GetArg("max_failed_shards");
        const auto& max_failed_shards_percent_str = request.GetArg("max_failed_shards_percent");
        const uint32_t max_failed_shards = max_failed_shards_str.empty() ? 0 : std::stoul(max_failed_shards_str);
        const uint32_t max_failed_shards_percent =
            max_failed_shards_percent_str.empty() ? 0 : std::stoul(max_failed_shards_percent_str);

        storages::redis::WaitConnectedMode mode = storages::redis::WaitConnectedMode::kNoWait;
        if (!mode_str.empty()) {
            if (mode_str == "no_wait") {
                mode = storages::redis::WaitConnectedMode::kNoWait;
            } else if (mode_str == "master_or_slave") {
                mode = storages::redis::WaitConnectedMode::kMasterOrSlave;
            } else if (mode_str == "master_and_slave") {
                mode = storages::redis::WaitConnectedMode::kMasterAndSlave;
            } else if (mode_str == "master") {
                mode = storages::redis::WaitConnectedMode::kMaster;
            } else if (mode_str == "slave") {
                mode = storages::redis::WaitConnectedMode::kSlave;
            } else {
                request.SetResponseStatus(server::http::HttpStatus::kBadRequest);
                return "Invalid mode";
            }
        }
        try {
            auto client = redis_component_.GetClient(db_name);
            if (client->IsReady(storages::redis::HealthCheckParams{mode, max_failed_shards, max_failed_shards_percent}))
            {
                return "OK";
            }
            return "NOT_READY";
        } catch (const std::exception& e) {
            request.SetResponseStatus(server::http::HttpStatus::kInternalServerError);
            throw;
        }
    }

private:
    components::Redis& redis_component_;
    storages::redis::CommandControl redis_cc_;
};

}  // namespace chaos

int main(int argc, char* argv[]) {
    const auto component_list =
        components::MinimalServerComponentList()
            .AppendComponentList(USERVER_NAMESPACE::dynamic_config::updater::ComponentList())
            .Append<chaos::KeyValue>("handler-cluster")
            .Append<chaos::IsReady>("handler-is-ready")
            .AppendComponentList(clients::http::ComponentList())
            .Append<components::Secdist>()
            .Append<components::DefaultSecdistProvider>()
            .Append<components::Redis>("key-value-database")
            .Append<components::TestsuiteSupport>()
            .Append<server::handlers::TestsControl>()
            .Append<server::handlers::Ping>()
            .Append<clients::dns::Component>();
    return utils::DaemonMain(argc, argv, component_list);
}
