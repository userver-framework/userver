#include "batch.hpp"

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
#include <vector>

namespace functional_tests {

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

namespace {

class BatchSelectInsert final : public server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-batch";

  BatchSelectInsert(const components::ComponentConfig& config,
                    const components::ComponentContext& context)
      : HttpHandlerJsonBase(config, context),
        sqlite_connection_(
            context.FindComponent<components::SQLite>("key-value-database")
                .GetConnection()) {
    sqlite_connection_->Execute(db::sql::kCreateTable.data());
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
      sqlite_connection_->ExecuteBulk(db::sql::kInsertKeyValue.data(), rows);
    } else {
      sqlite_connection_->ExecuteDecompose(db::sql::kInsertKeyValue.data(),
                                           rows.back());
    }

    auto records =
        sqlite_connection_->Execute(db::sql::kSelectAllKeyValue.data());

    std::vector<Row> result;
    for (size_t i = 0; i < records.Size(); ++i) {
      // TODO: Add conversion from sqlite::Row to user-defined structures
      result.push_back(Row{});
    }

    std::sort(
        result.begin(), result.end(),
        [](const auto& lhs, const auto& rhs) { return lhs.key < rhs.key; });

    formats::json::ValueBuilder builder{};
    builder["values"] = result;

    return builder.ExtractValue();
  }

  formats::json::Value GetValues() const {
    auto rows = sqlite_connection_->Execute(db::sql::kSelectAllKeyValue.data())
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

}  // namespace

void AppendBatch(userver::components::ComponentList& component_list) {
  component_list.Append<BatchSelectInsert>();
}

}  // namespace functional_tests
