#include <userver/utest/using_namespace_userver.hpp>

#include <userver/clients/dns/component.hpp>
#include <userver/components/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/storages/odbc.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/storages/secdist/provider_component.hpp>
#include <userver/utils/daemon_run.hpp>

namespace chaos {

class KeyValue final : public server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName{"handler-chaos"};

    KeyValue(const components::ComponentConfig& config, const components::ComponentContext& context)
        : server::handlers::HttpHandlerBase{config, context},
          odbc_{context.FindComponent<components::Odbc>("key-value-db").GetCluster()}
    {}

    std::string HandleRequestThrow(const server::http::HttpRequest& request, server::request::RequestContext&)
        const override {
        const auto& key = request.GetArg("key");
        if (key.empty()) {
            throw server::handlers::ClientError{server::handlers::ExternalBody{"No 'key' query argument"}};
        }

        switch (request.GetMethod()) {
            case server::http::HttpMethod::kGet:
                return GetValue(key, request);
            case server::http::HttpMethod::kPost:
                return PostValue(key, request);
            case server::http::HttpMethod::kDelete:
                return DeleteValue(key);
            default:
                throw server::handlers::ClientError{
                    server::handlers::ExternalBody{fmt::format("Unsupported method {}", request.GetMethod())}
                };
        }
    }

private:
    std::string GetValue(std::string_view key, const server::http::HttpRequest& request) const {
        auto result = odbc_->Execute(
            storages::odbc::ClusterHostType::kMaster,
            fmt::format("SELECT value FROM kv WHERE key = '{}'", key)
        );

        if (result.IsEmpty()) {
            request.SetResponseStatus(server::http::HttpStatus::kNotFound);
            return {};
        }

        return result[0][0].GetString();
    }

    std::string PostValue(std::string_view key, const server::http::HttpRequest& request) const {
        const auto& value = request.GetArg("value");
        if (value.empty()) {
            throw server::handlers::ClientError{server::handlers::ExternalBody{"No 'value' query argument"}};
        }

        odbc_->Execute(
            storages::odbc::ClusterHostType::kMaster,
            fmt::format(
                "INSERT INTO kv(key, value) VALUES ('{}', '{}') "
                "ON CONFLICT (key) DO UPDATE SET value = EXCLUDED.value",
                key,
                value
            )
        );

        request.SetResponseStatus(server::http::HttpStatus::kCreated);
        return std::string{value};
    }

    std::string DeleteValue(std::string_view key) const {
        odbc_->Execute(storages::odbc::ClusterHostType::kMaster, fmt::format("DELETE FROM kv WHERE key = '{}'", key));

        return {};
    }

    const std::shared_ptr<storages::odbc::Cluster> odbc_;
};

class KeyValueTrx final : public server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName{"handler-chaos-trx"};

    KeyValueTrx(const components::ComponentConfig& config, const components::ComponentContext& context)
        : server::handlers::HttpHandlerBase{config, context},
          odbc_{context.FindComponent<components::Odbc>("key-value-db").GetCluster()}
    {}

    std::string HandleRequestThrow(const server::http::HttpRequest& request, server::request::RequestContext&)
        const override {
        const auto& key = request.GetArg("key");
        if (key.empty()) {
            throw server::handlers::ClientError{server::handlers::ExternalBody{"No 'key' query argument"}};
        }

        switch (request.GetMethod()) {
            case server::http::HttpMethod::kGet:
                return GetValue(key, request);
            case server::http::HttpMethod::kPost:
                return PostValue(key, request);
            case server::http::HttpMethod::kDelete:
                return DeleteValue(key);
            default:
                throw server::handlers::ClientError{
                    server::handlers::ExternalBody{fmt::format("Unsupported method {}", request.GetMethod())}
                };
        }
    }

private:
    std::string GetValue(std::string_view key, const server::http::HttpRequest& request) const {
        auto trx = odbc_->Begin(storages::odbc::ClusterHostType::kMaster);
        auto result = trx.Execute(fmt::format("SELECT value FROM kv WHERE key = '{}'", key));
        trx.Commit();

        if (result.IsEmpty()) {
            request.SetResponseStatus(server::http::HttpStatus::kNotFound);
            return {};
        }

        return result[0][0].GetString();
    }

    std::string PostValue(std::string_view key, const server::http::HttpRequest& request) const {
        const auto& value = request.GetArg("value");
        if (value.empty()) {
            throw server::handlers::ClientError{server::handlers::ExternalBody{"No 'value' query argument"}};
        }

        auto trx = odbc_->Begin(storages::odbc::ClusterHostType::kMaster);
        trx.Execute(fmt::format(
            "INSERT INTO kv(key, value) VALUES ('{}', '{}') "
            "ON CONFLICT (key) DO UPDATE SET value = EXCLUDED.value",
            key,
            value
        ));
        trx.Commit();

        request.SetResponseStatus(server::http::HttpStatus::kCreated);
        return std::string{value};
    }

    std::string DeleteValue(std::string_view key) const {
        auto trx = odbc_->Begin(storages::odbc::ClusterHostType::kMaster);
        trx.Execute(fmt::format("DELETE FROM kv WHERE key = '{}'", key));
        trx.Commit();

        return {};
    }

    const std::shared_ptr<storages::odbc::Cluster> odbc_;
};

}  // namespace chaos

int main(int argc, char* argv[]) {
    const auto component_list =
        components::MinimalServerComponentList()
            .Append<chaos::KeyValue>()
            .Append<chaos::KeyValueTrx>()
            .Append<components::Secdist>()
            .Append<components::DefaultSecdistProvider>()
            .Append<clients::dns::Component>()
            .Append<components::Odbc>("key-value-db");

    return utils::DaemonMain(argc, argv, component_list);
}
