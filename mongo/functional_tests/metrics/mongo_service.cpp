#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/components/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/formats/bson/inline.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/handlers/server_monitor.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/storages/mongo/component.hpp>
#include <userver/storages/mongo/dist_lock_component_base.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/daemon_run.hpp>

#include <userver/utest/using_namespace_userver.hpp>

namespace metrics {

class KeyValue final : public server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-key-value";

  KeyValue(const components::ComponentConfig& config,
           const components::ComponentContext& context)
      : HttpHandlerBase(config, context),
        pool_(context.FindComponent<components::Mongo>("key-value-database")
                  .GetPool()) {}

  std::string HandleRequestThrow(
      const server::http::HttpRequest& request,
      server::request::RequestContext&) const override {
    switch (request.GetMethod()) {
      case server::http::HttpMethod::kGet:
        return GetValue(request);
      case server::http::HttpMethod::kPut:
        PutValue(request);
        return {};
      case server::http::HttpMethod::kDelete:
        DeleteValue(request);
        return {};
      default:
        ThrowUnsupportedHttpMethod(request);
    }
  }

 private:
  std::string GetValue(const server::http::HttpRequest& request) const;
  void PutValue(const server::http::HttpRequest& request) const;
  void DeleteValue(const server::http::HttpRequest& request) const;

  storages::mongo::PoolPtr pool_;
};

std::string KeyValue::GetValue(const server::http::HttpRequest& request) const {
  const auto& key = request.GetArg("key");

  using formats::bson::MakeDoc;
  auto coll = pool_->GetCollection("test");

  // Using 'count' instead of 'find', because Cursor may or may not provide
  // metrics, depending on the mongoc version.
  const auto count = coll.Count(MakeDoc("_id", MakeDoc("$eq", key)));

  return std::to_string(count);
}

void KeyValue::PutValue(const server::http::HttpRequest& request) const {
  const auto& key = request.GetArg("key");
  const auto& value = request.GetArg("value");

  using formats::bson::MakeDoc;
  auto coll = pool_->GetCollection("test");
  coll.InsertOne(MakeDoc("_id", key, "value", value));
}

void KeyValue::DeleteValue(const server::http::HttpRequest& request) const {
  const auto& key = request.GetArg("key");

  using formats::bson::MakeDoc;
  auto coll = pool_->GetCollection("test");
  coll.DeleteOne(MakeDoc("_id", key));
}

class DistlockMetrics final : public storages::mongo::DistLockComponentBase {
 public:
  static constexpr std::string_view kName = "component-distlock-metrics";

  DistlockMetrics(const components::ComponentConfig& config,
                  const components::ComponentContext& context)
      : storages::mongo::DistLockComponentBase(
            config, context,
            context.FindComponent<components::Mongo>("key-value-database")
                .GetPool()
                ->GetCollection("distlocks")) {
    Start();
  }

  ~DistlockMetrics() override { Stop(); }

  void DoWork() override {
    // noop
  }
};

}  // namespace metrics

int main(int argc, char* argv[]) {
  const auto component_list =
      components::MinimalServerComponentList()
          .Append<clients::dns::Component>()
          .Append<components::HttpClient>()
          .Append<components::TestsuiteSupport>()
          .Append<server::handlers::ServerMonitor>()
          .Append<server::handlers::TestsControl>()
          .Append<components::Mongo>("key-value-database")
          .Append<metrics::KeyValue>()
          .Append<metrics::DistlockMetrics>();
  return utils::DaemonMain(argc, argv, component_list);
}
