#include <userver/testsuite/testsuite_support.hpp>

#include <userver/utest/using_namespace_userver.hpp>

#include <userver/clients/http/component.hpp>
#include <userver/components/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/handlers/http_handler_json_base.hpp>
#include <userver/utils/daemon_run.hpp>
#include "userver/clients/dns/component.hpp"
#include "userver/server/http/http_method.hpp"

#include <userver/storages/sqlite/component.hpp>
#include <userver/storages/sqlite/connection.hpp>
#include <userver/storages/sqlite/options.hpp>
#include <userver/storages/sqlite/query.hpp>
#include <userver/storages/sqlite/result_set.hpp>
#include <userver/storages/sqlite/transaction.hpp>

namespace functional_tests {

class KeyValue final : public server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-key-value";

  KeyValue(const components::ComponentConfig& config,
           const components::ComponentContext& context);

  std::string HandleRequestThrow(
      const server::http::HttpRequest& request,
      server::request::RequestContext&) const override;

 private:
  std::string GetValue(std::string_view key,
                       const server::http::HttpRequest& request) const;
  std::string PostValue(std::string_view key,
                        const server::http::HttpRequest& request) const;
  std::string DeleteValue(std::string_view key) const;

  std::string UpdateValue(std::string_view key,
                          const server::http::HttpRequest& request) const;

  storages::sqlite::ConnectionPtr sqlite_connection_;
};

KeyValue::KeyValue(const components::ComponentConfig& config,
                   const components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      sqlite_connection_(
          context.FindComponent<components::SQLite>("key-value-database")
              .GetConnection()) {
  constexpr auto kCreateTable = R"~(
    CREATE TABLE IF NOT EXISTS key_value_table (
      key TEXT PRIMARY KEY,
      value TEXT
    )
  )~";
  sqlite_connection_->Execute(kCreateTable);
}

std::string KeyValue::HandleRequestThrow(
    const server::http::HttpRequest& request,
    server::request::RequestContext&) const {
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

std::string KeyValue::GetValue(std::string_view key,
                               const server::http::HttpRequest& request) const {
  storages::sqlite::ResultSet res = sqlite_connection_->Execute(
      "SELECT value FROM key_value_table WHERE key = ?", key);
  if (res.IsEmpty()) {
    request.SetResponseStatus(server::http::HttpStatus::kNotFound);
    return {};
  }

  return res.AsSingleRow<std::string>();
}

std::string KeyValue::PostValue(
    std::string_view key, const server::http::HttpRequest& request) const {
  const auto& value = request.GetArg("value");

  storages::sqlite::Transaction transaction =
      sqlite_connection_->Begin("sample_transaction_insert_key_value", {});

  auto res = transaction.Execute(
      "INSERT OR IGNORE INTO key_value_table (key, value) VALUES (?, ?)", key,
      value);
  if (res.RowsAffected()) {
    transaction.Commit();
    request.SetResponseStatus(server::http::HttpStatus::kCreated);
    return std::string{value};
  }

  res = transaction.Execute("SELECT value FROM key_value_table WHERE key = ?",
                            key);
  transaction.Rollback();

  auto result = res.AsSingleRow<std::string>();
  if (result != value) {
    request.SetResponseStatus(server::http::HttpStatus::kConflict);
  }

  return res.AsSingleRow<std::string>();
}

std::string KeyValue::UpdateValue(
    std::string_view key, const server::http::HttpRequest& request) const {
  const auto& value = request.GetArg("value");

  using storages::sqlite::TransactionOptions;

  storages::sqlite::Transaction transaction = sqlite_connection_->Begin(
      "sample_transaction_update_key_value",
      TransactionOptions{TransactionOptions::Mode::kImmediate});

  auto res = sqlite_connection_->Execute(
      "UPDATE OR IGNORE key_value_table SET value = ? WHERE key = ?", value,
      key);

  if (res.RowsAffected()) {
    transaction.Commit();
    request.SetResponseStatus(server::http::HttpStatus::kCreated);
    return std::string{value};
  }
  res = transaction.Execute("SELECT value FROM key_value_table WHERE key = ?",
                            key);
  transaction.Rollback();

  auto result = res.AsSingleRow<std::string>();
  if (result != value) {
    request.SetResponseStatus(server::http::HttpStatus::kConflict);
  }

  return res.AsSingleRow<std::string>();
}

std::string KeyValue::DeleteValue(std::string_view key) const {
  const storages::sqlite::Query kDeleteValue{
      "DELETE FROM key_value_table WHERE key = ?",
      storages::sqlite::Query::Name{"sample_delete_value"},
  };

  auto res = sqlite_connection_->Execute(kDeleteValue, key);
  return std::to_string(res.RowsAffected());
}

struct Row final {
  std::string key;
  std::string value;
};

formats::json::Value Serialize(const Row& row,
                               formats::serialize::To<formats::json::Value>) {
  formats::json::ValueBuilder builder{};
  builder["key"] = row.key;
  builder["value"] = row.value;

  return builder.ExtractValue();
}

Row Parse(const formats::json::Value& json, formats::parse::To<Row>) {
  return {json["key"].As<std::string>(), json["value"].As<std::string>()};
}

class BatchSelectInsert final : public server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-batch";

  BatchSelectInsert(const components::ComponentConfig& config,
                    const components::ComponentContext& context)
      : HttpHandlerJsonBase(config, context),
        sqlite_connection_(
            context.FindComponent<components::SQLite>("key-value-database")
                .GetConnection()) {
    constexpr auto kCreateTable = R"~(
    CREATE TABLE IF NOT EXISTS key_value_table (
      key TEXT PRIMARY KEY,
      value TEXT
    )
  )~";
    sqlite_connection_->Execute(kCreateTable);
  }

  formats::json::Value HandleRequestJsonThrow(
      const server::http::HttpRequest& request,
      const formats::json::Value& request_json,
      server::request::RequestContext&) const final {
    request.GetHttpResponse().SetContentType(
        http::content_type::kApplicationJson);
    switch (request.GetMethod()) {
      case server::http::HttpMethod::kGet:
        return GetValues();
      case server::http::HttpMethod::kPost:
        return InsertValues(request_json);
      default:
        throw server::handlers::ClientError(server::handlers::ExternalBody{
            fmt::format("Unsupported method {}", request.GetMethod())});
    }
  }

  formats::json::Value InsertValues(
      const formats::json::Value& json_request) const {
    const auto rows = json_request["data"].As<std::vector<Row>>();
    if (rows.empty()) {
      return {};
    }

    if (rows.size() > 1) {
      sqlite_connection_->ExecuteBulk(
          "INSERT INTO key_value_table(`key`, value) VALUES(?, ?)", rows);
    } else {
      sqlite_connection_->ExecuteDecompose(
          "INSERT INTO key_value_table(`key`, value) VALUES(?, ?)",
          rows.back());
    }

    return {};
  }

  formats::json::Value GetValues() const {
    auto rows = sqlite_connection_->Execute("SELECT * FROM key_value_table")
                    .AsVector<Row>();
    std::sort(rows.begin(), rows.end(), [](const auto& lhs, const auto& rhs) {
      return lhs.key < rhs.key;
    });

    formats::json::ValueBuilder builder{};
    builder["values"] = rows;

    return builder.ExtractValue();
  }

 private:
  storages::sqlite::ConnectionPtr sqlite_connection_;
};

}  // namespace functional_tests

int main(int argc, char* argv[]) {
  const auto component_list =
      components::MinimalServerComponentList()
          .Append<functional_tests::KeyValue>()
          .Append<functional_tests::BatchSelectInsert>()
          .Append<components::SQLite>("key-value-database")
          .Append<components::TestsuiteSupport>()
          .Append<clients::dns::Component>();
  return utils::DaemonMain(argc, argv, component_list);
}
