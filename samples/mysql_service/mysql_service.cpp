#include <userver/clients/dns/component.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utest/using_namespace_userver.hpp>

#include <userver/components/minimal_server_component_list.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/server/handlers/http_handler_json_base.hpp>
#include <userver/storages/secdist/component.hpp>
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

struct KeyValueCachePolicy final {
  static constexpr std::string_view kName{"key-value-cache"};

  using ValueType = Row;

  static constexpr auto kKeyMember = &Row::key;

  static constexpr std::string_view kQuery{
      "SELECT `Key`, Value FROM key_value_table"};

  static constexpr std::nullptr_t kUpdatedField{};
};
using KeyValueDbCache = components::MySQLCache<KeyValueCachePolicy>;

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
      mysql_{context.FindComponent<components::MySQL>("test").GetCluster()} {}

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
  mysql_->InsertMany({},
                     "INSERT INTO key_value_table(`key`, value) VALUES(?, ?)",
                     json_request["data"].As<std::vector<Row>>());

  return {};
}

formats::json::Value KeyValue::GetValues() const {
  formats::json::ValueBuilder builder{};
  builder["values"] =
      mysql_
          ->Select(userver::storages::mysql::ClusterHostType::kMaster,
                   "SELECT `key`, value FROM key_value_table")
          .AsVector<Row>();

  return builder.ExtractValue();
}

class KeyValueCacheHandler final
    : public server::handlers::HttpHandlerJsonBase {
 public:
  static constexpr std::string_view kName = "handler-key-value-cache";

  KeyValueCacheHandler(const components::ComponentConfig& config,
                       const components::ComponentContext& context)
      : server::handlers::HttpHandlerJsonBase{config, context},
        cache_{context.FindComponent<KeyValueDbCache>()} {}

  formats::json::Value HandleRequestJsonThrow(
      const server::http::HttpRequest&, const formats::json::Value&,
      server::request::RequestContext&) const final;

 private:
  const KeyValueDbCache& cache_;
};

formats::json::Value KeyValueCacheHandler::HandleRequestJsonThrow(
    const server::http::HttpRequest&, const formats::json::Value&,
    server::request::RequestContext&) const {
  auto cache_ptr = cache_.Get();

  formats::json::ValueBuilder builder{};
  std::vector<Row> rows(cache_ptr->size());
  std::transform(cache_ptr->begin(), cache_ptr->end(), rows.begin(),
                 [](const auto& pair) { return pair.second; });
  builder["values"] = rows;

  return builder.ExtractValue();
}

int main(int argc, char* argv[]) {
  const auto component_list = components::MinimalServerComponentList()
                                  .Append<KeyValueDbCache>()
                                  .Append<KeyValue>()
                                  .Append<KeyValueCacheHandler>()
                                  .Append<components::MySQL>("test")
                                  .Append<userver::components::Secdist>()
                                  .Append<components::TestsuiteSupport>()
                                  .Append<clients::dns::Component>();

  return utils::DaemonMain(argc, argv, component_list);
}

}  // namespace samples::mysql

int main(int argc, char* argv[]) { return samples::mysql::main(argc, argv); }
