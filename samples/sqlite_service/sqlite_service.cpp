#include <userver/clients/dns/component.hpp>
#include <userver/testsuite/testsuite_support.hpp>

#include <userver/utest/using_namespace_userver.hpp>

/// [SQLite service sample - component]
#include <userver/clients/http/component.hpp>
#include <userver/components/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/utils/daemon_run.hpp>

#include <userver/storages/sqlite/client.hpp>
#include <userver/storages/sqlite/component.hpp>
#include "userver/storages/sqlite/operation_types.hpp"

namespace {

inline constexpr std::string_view kCreateTable = R"~(
    CREATE TABLE IF NOT EXISTS key_value_table (
    key TEXT PRIMARY KEY,
    value TEXT
    )
)~";

inline constexpr std::string_view kSelectValueByKey = R"~(
    SELECT value FROM key_value_table WHERE key = ?
)~";

inline constexpr std::string_view kInsertKeyValue = R"~(
    INSERT OR IGNORE INTO key_value_table (key, value) VALUES (?, ?)
)~";

inline constexpr std::string_view kDeleteKeyValue = R"~(
    DELETE FROM key_value_table WHERE key = ?
)~";

}  // namespace

namespace samples_sqlite_service::sqlite {

class KeyValue final : public server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-key-value";

    KeyValue(const components::ComponentConfig& config, const components::ComponentContext& context);

    std::string HandleRequest(server::http::HttpRequest& request, server::request::RequestContext&) const final;

private:
    std::string GetValue(std::string_view key, const server::http::HttpRequest& request) const;
    std::string PostValue(std::string_view key, const server::http::HttpRequest& request) const;
    std::string DeleteValue(std::string_view key) const;

    storages::sqlite::ClientPtr sqlite_client_;
};

}  // namespace samples_sqlite_service::sqlite
/// [SQLite service sample - component]

namespace samples_sqlite_service::sqlite {

/// [SQLite service sample - component constructor]
KeyValue::KeyValue(const components::ComponentConfig& config, const components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      sqlite_client_(context.FindComponent<components::SQLite>("key-value-database").GetClient()) {
    sqlite_client_->Execute(storages::sqlite::OperationType::kReadWrite, kCreateTable.data());
}
/// [SQLite service sample - component constructor]

/// [SQLite service sample - HandleRequestThrow]
std::string KeyValue::HandleRequest(server::http::HttpRequest& request, server::request::RequestContext&) const {
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
                fmt::format("Unsupported method {}", request.GetMethod())});
    }
}
/// [SQLite service sample - HandleRequestThrow]

/// [SQLite service sample - GetValue]
std::string KeyValue::GetValue(std::string_view key, const server::http::HttpRequest& request) const {
    auto res = sqlite_client_->Execute(storages::sqlite::OperationType::kReadOnly, kSelectValueByKey.data(), key)
                   .AsOptionalSingleField<std::string>();
    if (!res.has_value()) {
        request.SetResponseStatus(server::http::HttpStatus::kNotFound);
        return {};
    }

    return res.value();
}
/// [SQLite service sample - GetValue]

/// [SQLite service sample - PostValue]
std::string KeyValue::PostValue(std::string_view key, const server::http::HttpRequest& request) const {
    const auto& value = request.GetArg("value");

    storages::sqlite::Transaction transaction = sqlite_client_->Begin(storages::sqlite::OperationType::kReadWrite, {});

    auto res = transaction.Execute(kInsertKeyValue.data(), key, value).AsExecutionResult();
    if (res.rows_affected) {
        transaction.Commit();
        request.SetResponseStatus(server::http::HttpStatus::kCreated);
        return std::string{value};
    }

    auto trx_res = transaction.Execute(kSelectValueByKey.data(), key).AsSingleField<std::string>();
    transaction.Rollback();
    if (value != trx_res) {
        request.SetResponseStatus(server::http::HttpStatus::kConflict);
    }
    return trx_res;
}
/// [SQLite service sample - PostValue]

/// [SQLite service sample - DeleteValue]
std::string KeyValue::DeleteValue(std::string_view key) const {
    const storages::sqlite::Query kDeleteValue{kDeleteKeyValue.data()};

    auto res =
        sqlite_client_->Execute(storages::sqlite::OperationType::kReadWrite, kDeleteValue, key).AsExecutionResult();
    return std::to_string(res.rows_affected);
}
/// [SQLite service sample - DeleteValue]

}  // namespace samples_sqlite_service::sqlite

/// [SQLite service sample - main]
int main(int argc, char* argv[]) {
    const auto component_list = components::MinimalServerComponentList()
                                    .Append<samples_sqlite_service::sqlite::KeyValue>()
                                    .Append<components::SQLite>("key-value-database")
                                    .Append<components::HttpClient>()
                                    .Append<server::handlers::TestsControl>()
                                    .Append<components::TestsuiteSupport>()
                                    .Append<clients::dns::Component>();
    return utils::DaemonMain(argc, argv, component_list);
}
/// [SQLite service sample - main]
