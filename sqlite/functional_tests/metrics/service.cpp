#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/components/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/handlers/http_handler_json_base.hpp>
#include <userver/server/handlers/server_monitor.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/server/http/http_method.hpp>
#include <userver/testsuite/testpoint.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utest/using_namespace_userver.hpp>
#include <userver/utils/daemon_run.hpp>

#include <userver/storages/sqlite.hpp>

namespace sqlite::metrics {

struct KeyValueRow {
    std::string key;
    std::string value;
};

class SQLiteHandler final : public server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-metrics-sqlite";

    SQLiteHandler(const components::ComponentConfig& config, const components::ComponentContext& context);

    std::string HandleRequestThrow(const server::http::HttpRequest& request, server::request::RequestContext&)
        const override;

private:
    storages::sqlite::ClientPtr sqlite_client_;
};

SQLiteHandler::SQLiteHandler(const components::ComponentConfig& config, const components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      sqlite_client_(context.FindComponent<components::SQLite>("key-value-database").GetClient()) {
    sqlite_client_->Execute(
        storages::sqlite::OperationType::kReadWrite,
        R"~(
                                    CREATE TABLE IF NOT EXISTS key_value_table (
                                    key PRIMARY KEY,
                                    value TEXT
                                    ))~"
    );
}

std::string
SQLiteHandler::HandleRequestThrow(const server::http::HttpRequest& request, server::request::RequestContext&) const {
    const auto& key = request.GetArg("key");
    const auto& value = request.GetArg("value");
    if (key.empty()) {
        request.GetHttpResponse().SetStatus(server::http::HttpStatus::kBadRequest);
    }

    if (request.GetMethod() == server::http::HttpMethod::kPost) {
        if (value.empty()) {
            request.GetHttpResponse().SetStatus(server::http::HttpStatus::kBadRequest);
            return "Bad Request";
        }
        KeyValueRow data{key, value};
        sqlite_client_->ExecuteDecompose(
            storages::sqlite::OperationType::kReadWrite, "INSERT INTO key_value_table VALUES (?, ?)", data
        );
    } else if (request.GetMethod() == server::http::HttpMethod::kDelete) {
        const storages::sqlite::Query query{"DELETE FROM key_value_table WHERE key = ?"};
        sqlite_client_->Execute(storages::sqlite::OperationType::kReadWrite, query, key);
        return fmt::format("Deleted by key: {}", key);
    }
    const storages::sqlite::Query query{"SELECT key, value FROM key_value_table WHERE key = ?"};

    const auto result = sqlite_client_->Execute(storages::sqlite::OperationType::kReadOnly, query, key)
                            .AsOptionalSingleRow<KeyValueRow>();

    std::string ret{};
    if (result.has_value()) {
        ret += fmt::format("{}: {}", result.value().key, result.value().value);
    }
    return ret;
}

}  // namespace sqlite::metrics

int main(int argc, char* argv[]) {
    const auto component_list = components::MinimalServerComponentList()
                                    .Append<server::handlers::ServerMonitor>()
                                    .Append<sqlite::metrics::SQLiteHandler>()
                                    .Append<components::HttpClient>()
                                    .Append<components::SQLite>("key-value-database")
                                    .Append<components::TestsuiteSupport>()
                                    .Append<server::handlers::TestsControl>()
                                    .Append<clients::dns::Component>();
    return utils::DaemonMain(argc, argv, component_list);
}
