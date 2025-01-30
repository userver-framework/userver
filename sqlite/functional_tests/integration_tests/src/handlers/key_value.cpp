#include "key_value.hpp"

#include <userver/clients/http/component.hpp>
#include <userver/components/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/handlers/http_handler_json_base.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utest/using_namespace_userver.hpp>
#include <userver/utils/daemon_run.hpp>
#include "userver/server/http/http_method.hpp"

#include <userver/storages/sqlite/component.hpp>
#include <userver/storages/sqlite/connection.hpp>
#include <userver/storages/sqlite/options.hpp>
#include <userver/storages/sqlite/query.hpp>
#include <userver/storages/sqlite/result_set.hpp>
#include <userver/storages/sqlite/transaction.hpp>

#include <db/sql.hpp>

namespace functional_tests {

namespace {

constexpr std::string_view kInsertKeyValueTransactionName =
    "sample_transaction_insert_key_value";
constexpr std::string_view kUpdateKeyValueTransactionName =
    "sample_transaction_update_key_value";
constexpr std::string_view kDeleteQueryName = "sample_delete_value";

class KeyValue final : public server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-key-value";

  KeyValue(const components::ComponentConfig& config,
           const components::ComponentContext& context)
      : HttpHandlerBase(config, context),
        sqlite_connection_(
            context.FindComponent<components::SQLite>("key-value-database")
                .GetConnection()) {
    sqlite_connection_->Execute(db::sql::kCreateTable.data());
  }

  std::string HandleRequestThrow(const server::http::HttpRequest& request,
                                 server::request::RequestContext&) const final {
    const auto& key = request.GetArg("key");
    if (key.empty()) {
      throw server::handlers::ClientError(
          server::handlers::ExternalBody{"No 'key' query argument"});
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
  std::string GetValue(std::string_view key,
                       const server::http::HttpRequest& request) const {
    auto res =
        sqlite_connection_->Execute(db::sql::kSelectValueByKey.data(), key)
            .AsOptionalSingleField<std::string>();
    if (!res.has_value()) {
      request.SetResponseStatus(server::http::HttpStatus::kNotFound);
      return {};
    }

    return res.value();
  }

  std::string PostValue(std::string_view key,
                        const server::http::HttpRequest& request) const {
    const auto& value = request.GetArg("value");

    storages::sqlite::Transaction transaction =
        sqlite_connection_->Begin(kInsertKeyValueTransactionName.data(), {});

    auto res = transaction.Execute(db::sql::kInsertKeyValue.data(), key, value)
                   .AsExecutionResult();
    if (res.rows_affected) {
      transaction.Commit();
      request.SetResponseStatus(server::http::HttpStatus::kCreated);
      return std::string{value};
    }

    auto trx_res = transaction.Execute(db::sql::kSelectValueByKey.data(), key)
                       .AsSingleField<std::string>();
    transaction.Rollback();
    if (value != trx_res) {
      request.SetResponseStatus(server::http::HttpStatus::kConflict);
    }
    return trx_res;
  }

  std::string UpdateValue(std::string_view key,
                          const server::http::HttpRequest& request) const {
    const auto& value = request.GetArg("value");

    using storages::sqlite::TransactionOptions;

    storages::sqlite::Transaction transaction = sqlite_connection_->Begin(
        kUpdateKeyValueTransactionName.data(),
        TransactionOptions{TransactionOptions::Mode::kImmediate});

    auto res =
        sqlite_connection_->Execute(db::sql::kUpdateKeyValue.data(), value, key)
            .AsExecutionResult();
    if (res.rows_affected) {
      transaction.Commit();
      request.SetResponseStatus(server::http::HttpStatus::kCreated);
      return std::string{value};
    }

    auto trx_res = transaction.Execute(db::sql::kSelectValueByKey.data(), key)
                       .AsOptionalSingleField<std::string>();
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
    const storages::sqlite::Query kDeleteValue{
        db::sql::kDeleteKeyValue.data(),
        storages::sqlite::Query::Name{kDeleteQueryName},
    };

    auto res =
        sqlite_connection_->Execute(kDeleteValue, key).AsExecutionResult();
    return std::to_string(res.rows_affected);
  }

  storages::sqlite::ConnectionPtr sqlite_connection_;
};

}  // namespace

void AppendKeyValue(userver::components::ComponentList& component_list) {
  component_list.Append<KeyValue>();
}

}  // namespace functional_tests
