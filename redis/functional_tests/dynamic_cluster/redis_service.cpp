#include <chrono>
#include <string>
#include <string_view>

#include <fmt/format.h>

#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component_list.hpp>
#include <userver/components/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/dynamic_config/client/component.hpp>
#include <userver/dynamic_config/updater/component.hpp>
#include <userver/formats/json.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/handlers/server_monitor.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/storages/redis/client.hpp>
#include <userver/storages/redis/dynamic_component.hpp>
#include <userver/storages/redis/exception.hpp>
#include <userver/storages/redis/wait_connected_mode.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/storages/secdist/provider_component.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utest/using_namespace_userver.hpp>  // IWYU pragma: keep
#include <userver/utils/daemon_run.hpp>
#include <userver/yaml_config/merge_schemas.hpp>
#include "userver/server/http/http_method.hpp"

namespace chaos {

class ClusterCommand final : public server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-cluster-command";

    ClusterCommand(const components::ComponentConfig& config, const components::ComponentContext& context);
    std::string HandleRequestThrow(const server::http::HttpRequest& request, server::request::RequestContext&)
        const override;

private:
    components::DynamicRedis& redis_component_;
    storages::redis::CommandControl redis_cc_;
};

ClusterCommand::ClusterCommand(const components::ComponentConfig& config, const components::ComponentContext& context)
    : server::handlers::HttpHandlerBase(config, context),
      redis_component_{context.FindComponent<components::DynamicRedis>("key-value-database")}
{
    // To prevent replication related flaps
    redis_cc_.force_request_to_master = true;
}

std::string ClusterCommand::HandleRequestThrow(
    const server::http::HttpRequest& request,
    server::request::RequestContext& /*context*/
) const {
    const auto& dbname = request.GetArg("dbname");
    if (dbname.empty()) {
        throw server::handlers::ClientError(server::handlers::ExternalBody{"No 'dbname' query argument"});
    }
    const auto& key = request.GetArg("key");
    if (key.empty()) {
        throw server::handlers::ClientError(server::handlers::ExternalBody{"No 'key' query argument"});
    }

    storages::redis::RedisWaitConnected wait_connected;
    wait_connected.mode = storages::redis::WaitConnectedMode::kSlave;
    wait_connected.timeout = std::chrono::seconds(60);
    wait_connected.throw_on_fail = true;

    try {
        auto client = redis_component_.GetDynamicClient(dbname, wait_connected);
        auto result = client->Get(std::string(key), redis_cc_).Get();
        if (!result) {
            request.SetResponseStatus(server::http::HttpStatus::kNotFound);
            return {};
        }
        return *result;
    } catch (const std::out_of_range& e) {
        request.SetResponseStatus(server::http::HttpStatus::kForbidden);
        return {};
    }
}

class ClusterControl final : public server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-redis";
    ClusterControl(const components::ComponentConfig& config, const components::ComponentContext& context);
    std::string HandleRequestThrow(const server::http::HttpRequest& request, server::request::RequestContext&)
        const override;

private:
    components::DynamicRedis& redis_component_;
    storages::redis::CommandControl redis_cc_;
};

ClusterControl::ClusterControl(const components::ComponentConfig& config, const components::ComponentContext& context)
    : server::handlers::HttpHandlerBase(config, context),
      redis_component_{context.FindComponent<components::DynamicRedis>("key-value-database")}
{}

std::string ClusterControl::HandleRequestThrow(
    const server::http::HttpRequest& request,
    server::request::RequestContext& /*context*/
) const {
    if (request.GetMethod() == server::http::HttpMethod::kPost) {
        const auto& body = formats::json::FromString(request.RequestBody());
        const auto& dbname = body["dbname"].As<std::optional<std::string>>();
        if (!dbname) {
            request.SetResponseStatus(server::http::HttpStatus::kBadRequest);
            return "Invalid argument: No 'dbname' in body";
        }

        storages::redis::DynamicSettings settings;
        const auto& hosts = body["hosts"];
        for (const auto& host_port : hosts) {
            const auto host = host_port["host"].As<std::string>();
            const auto port = host_port["port"].As<int>();
            settings.sentinels.emplace_back(host, port);
        }
        settings.password = body["password"].As<storages::redis::Password>();
        settings.sharding_strategy = storages::redis::ToShardingStrategy(body["sharding_strategy"].As<std::string>());
        settings.shards = {"test_master0"};

        try {
            if (!redis_component_.AddClient(*dbname, settings)) {
                request.SetResponseStatus(server::http::HttpStatus::kConflict);
                return "Client already exists";
            }
            return "ok";
        } catch (const std::invalid_argument& e) {
            request.SetResponseStatus(server::http::HttpStatus::kBadRequest);
            return fmt::format("Invalid argument: {}", e.what());
        } catch (const std::exception& e) {
            request.SetResponseStatus(server::http::HttpStatus::kInternalServerError);
            return fmt::format("Error: {}", e.what());
        }
    }

    if (request.GetMethod() == server::http::HttpMethod::kDelete) {
        const auto& dbname = request.GetArg("dbname");
        if (dbname.empty()) {
            request.SetResponseStatus(server::http::HttpStatus::kBadRequest);
            return "Invalid argument: No 'dbname' query argument";
        }
        if (!redis_component_.RemoveClient(dbname)) {
            request.SetResponseStatus(server::http::HttpStatus::kNotFound);
            return "Not found";
        }
        return "ok";
    }

    throw server::handlers::ClientError(server::handlers::ExternalBody{"Wrong method"});
}

}  // namespace chaos

int main(int argc, char* argv[]) {
    const auto component_list =
        components::MinimalServerComponentList()
            .Append<chaos::ClusterCommand>("handler-cluster-command")
            .Append<chaos::ClusterControl>("handler-cluster-control")
            .Append<server::handlers::ServerMonitor>()
            .AppendComponentList(clients::http::ComponentList())
            .Append<components::Secdist>()
            .Append<components::DefaultSecdistProvider>()
            .Append<components::DynamicRedis>("key-value-database")
            .Append<components::TestsuiteSupport>()
            .Append<server::handlers::TestsControl>()
            .Append<clients::dns::Component>();
    return utils::DaemonMain(argc, argv, component_list);
}
