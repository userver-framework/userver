#include <userver/testsuite/testsuite_support.hpp>

#include <userver/utest/using_namespace_userver.hpp>

#include <userver/clients/http/component.hpp>
#include <userver/components/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/utils/daemon_run.hpp>
#include "userver/clients/dns/component.hpp"

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

std::string KeyValue::DeleteValue(std::string_view key) const {
  const storages::sqlite::Query kDeleteValue{
      "DELETE FROM key_value_table WHERE key = ?",
      storages::sqlite::Query::Name{"sample_delete_value"},
  };

  auto res = sqlite_connection_->Execute(kDeleteValue, key);
  return std::to_string(res.RowsAffected());
}

}  // namespace functional_tests

int main(int argc, char* argv[]) {
  const auto component_list =
      components::MinimalServerComponentList()
          .Append<functional_tests::KeyValue>()
          .Append<components::SQLite>("key-value-database")
          .Append<components::TestsuiteSupport>()
          .Append<clients::dns::Component>();
  return utils::DaemonMain(argc, argv, component_list);
}
