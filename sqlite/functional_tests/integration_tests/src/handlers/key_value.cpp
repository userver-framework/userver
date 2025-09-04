#include "key_value.hpp"

#include <userver/clients/http/component.hpp>
#include <userver/components/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/handlers/http_handler_json_base.hpp>
#include <userver/server/http/http_method.hpp>
#include <userver/utils/daemon_run.hpp>

#include <userver/storages/sqlite.hpp>

#include <db/sql.hpp>

namespace functional_tests {

namespace {

class KeyValue final : public server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-key-value";

    KeyValue(const components::ComponentConfig& config, const components::ComponentContext& context)
        : HttpHandlerBase(config, context),
          sqlite_client_(context.FindComponent<components::SQLite>("key-value-database").GetClient()) {
        sqlite_client_->Execute(storages::sqlite::OperationType::kReadWrite, db::sql::kCreateTable.data());
    }

    std::string HandleRequestThrow(const server::http::HttpRequest& request, server::request::RequestContext&)
        const final {
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
            case server::http::HttpMethod::kPut:
                return UpdateValue(key, request);
            default:
                throw server::handlers::ClientError(server::handlers::ExternalBody{
                    fmt::format("Unsupported method {}", request.GetMethod())});
        }
    }

private:
    std::string GetValue(std::string_view key, const server::http::HttpRequest& request) const {
        auto res =
            sqlite_client_->Execute(storages::sqlite::OperationType::kReadOnly, db::sql::kSelectValueByKey.data(), key)
                .AsOptionalSingleField<std::string>();
        if (!res.has_value()) {
            request.SetResponseStatus(server::http::HttpStatus::kNotFound);
            return {};
        }

        return res.value();
    }

    std::string PostValue(std::string_view key, const server::http::HttpRequest& request) const {
        const auto& value = request.GetArg("value");

        storages::sqlite::Transaction transaction =
            sqlite_client_->Begin(storages::sqlite::OperationType::kReadWrite, {});

        auto res = transaction.Execute(db::sql::kInsertKeyValue.data(), key, value).AsExecutionResult();
        if (res.rows_affected) {
            transaction.Commit();
            request.SetResponseStatus(server::http::HttpStatus::kCreated);
            return std::string{value};
        }

        auto trx_res = transaction.Execute(db::sql::kSelectValueByKey.data(), key).AsSingleField<std::string>();
        transaction.Rollback();
        if (value != trx_res) {
            request.SetResponseStatus(server::http::HttpStatus::kConflict);
        }
        return trx_res;
    }

    std::string UpdateValue(std::string_view key, const server::http::HttpRequest& request) const {
        const auto& value = request.GetArg("value");

        using storages::sqlite::settings::TransactionOptions;

        storages::sqlite::Transaction transaction = sqlite_client_->Begin(
            storages::sqlite::OperationType::kReadWrite, TransactionOptions{TransactionOptions::LockingMode::kImmediate}
        );

        auto res = transaction.Execute(db::sql::kUpdateKeyValue.data(), value, key).AsExecutionResult();
        if (res.rows_affected) {
            transaction.Commit();
            request.SetResponseStatus(server::http::HttpStatus::kCreated);
            return std::string{value};
        }

        auto trx_res = transaction.Execute(db::sql::kSelectValueByKey.data(), key).AsOptionalSingleField<std::string>();
        if (!trx_res.has_value()) {
            request.SetResponseStatus(server::http::HttpStatus::kNotFound);
            return {};
        }
        transaction.Rollback();

        auto result = trx_res.value();
        if (result != value) {
            request.SetResponseStatus(server::http::HttpStatus::kConflict);
        }

        return result;
    }

    std::string DeleteValue(std::string_view key) const {
        const storages::sqlite::Query kDeleteValue{db::sql::kDeleteKeyValue.data()};

        auto res =
            sqlite_client_->Execute(storages::sqlite::OperationType::kReadWrite, kDeleteValue, key).AsExecutionResult();
        return std::to_string(res.rows_affected);
    }

    storages::sqlite::ClientPtr sqlite_client_;
};

}  // namespace

void AppendKeyValue(components::ComponentList& component_list) { component_list.Append<KeyValue>(); }

}  // namespace functional_tests
