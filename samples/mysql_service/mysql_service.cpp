#include <userver/clients/dns/component.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utest/using_namespace_userver.hpp>

#include <userver/clients/http/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/server/handlers/http_handler_json_base.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/storages/secdist/provider_component.hpp>
#include <userver/utils/daemon_run.hpp>

#include <userver/storages/mysql.hpp>

namespace samples::mysql {

struct Row final {
  std::int32_t key{};
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
  return {json["key"].As<std::int32_t>(), json["value"].As<std::string>()};
}

class KeyValue final : public server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-key-value";

  KeyValue(const components::ComponentConfig& config,
           const components::ComponentContext& context);

  formats::json::Value HandleRequestJsonThrow(
      const server::http::HttpRequest& request,
      const formats::json::Value& request_json,
      server::request::RequestContext&) const final;

 private:
  formats::json::Value GetValues() const;

  formats::json::Value InsertValues(
      const formats::json::Value& json_request) const;

  std::shared_ptr<storages::mysql::Cluster> mysql_;
};

KeyValue::KeyValue(const components::ComponentConfig& config,
                   const components::ComponentContext& context)
    : server::handlers::HttpHandlerJsonBase{config, context},
      mysql_{
          context
              .FindComponent<storages::mysql::Component>("sample-sql-component")
              .GetCluster()} {}

formats::json::Value KeyValue::HandleRequestJsonThrow(
    const server::http::HttpRequest& request,
    const formats::json::Value& request_json,
    server::request::RequestContext&) const {
  switch (request.GetMethod()) {
    case server::http::HttpMethod::kPost:
      return InsertValues(request_json);
    case server::http::HttpMethod::kGet:
      return GetValues();
    default:
      throw server::handlers::ClientError(server::handlers::ExternalBody{
          fmt::format("Unsupported method {}", request.GetMethod())});
  }
}

formats::json::Value KeyValue::InsertValues(
    const formats::json::Value& json_request) const {
  const auto rows = json_request["data"].As<std::vector<Row>>();
  if (rows.empty()) {
    return {};
  }

  if (rows.size() > 1) {
    mysql_->ExecuteBulk(
        storages::mysql::ClusterHostType::kPrimary,
        "INSERT INTO key_value_table(`key`, value) VALUES(?, ?)", rows);
  } else {
    mysql_->ExecuteDecompose(
        storages::mysql::ClusterHostType::kPrimary,
        "INSERT INTO key_value_table(`key`, value) VALUES(?, ?)", rows.back());
  }

  return {};
}

formats::json::Value KeyValue::GetValues() const {
  auto rows = mysql_
                  ->Execute(storages::mysql::ClusterHostType::kPrimary,
                            "SELECT `key`, value FROM key_value_table")
                  .AsVector<Row>();
  std::sort(rows.begin(), rows.end(),
            [](const auto& lhs, const auto& rhs) { return lhs.key < rhs.key; });

  formats::json::ValueBuilder builder{};
  builder["values"] = rows;

  return builder.ExtractValue();
}

int main(int argc, char* argv[]) {
  const auto component_list =
      components::MinimalServerComponentList()
          .Append<KeyValue>()
          .Append<storages::mysql::Component>("sample-sql-component")
          .Append<components::Secdist>()
          .Append<components::DefaultSecdistProvider>()
          .Append<components::TestsuiteSupport>()
          .Append<server::handlers::TestsControl>()
          .Append<components::HttpClient>()
          .Append<clients::dns::Component>();

  return utils::DaemonMain(argc, argv, component_list);
}

}  // namespace samples::mysql

int main(int argc, char* argv[]) { return samples::mysql::main(argc, argv); }
