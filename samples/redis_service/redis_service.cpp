#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utest/using_namespace_userver.hpp>  // IWYU pragma: keep

#include <fmt/format.h>
#include <string>
#include <string_view>

/// [Redis service sample - component]
#include <userver/clients/dns/component.hpp>
#include <userver/components/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/storages/redis/client.hpp>
#include <userver/storages/redis/component.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/storages/secdist/provider_component.hpp>
#include <userver/utils/daemon_run.hpp>

namespace samples::redis {
class EvalSha final : public server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-script";

  EvalSha(const components::ComponentConfig& config,
          const components::ComponentContext& context);

  std::string HandleRequestThrow(
      const server::http::HttpRequest& request,
      server::request::RequestContext&) const override;

 private:
  std::string EvalShaRequest(const server::http::HttpRequest& request) const;
  std::string ScriptLoad(const server::http::HttpRequest& request) const;

  storages::redis::ClientPtr redis_client_;
  storages::redis::CommandControl redis_cc_;
};

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

  storages::redis::ClientPtr redis_client_;
  storages::redis::CommandControl redis_cc_;
};

}  // namespace samples::redis
/// [Redis service sample - component]

namespace samples::redis {

/// [Redis service sample - component constructor]
KeyValue::KeyValue(const components::ComponentConfig& config,
                   const components::ComponentContext& context)
    : server::handlers::HttpHandlerBase(config, context),
      redis_client_{
          context.FindComponent<components::Redis>("key-value-database")
              .GetClient("taxi-tmp")},
      redis_cc_{std::chrono::seconds{15}, std::chrono::seconds{60}, 4} {}
/// [Redis service sample - component constructor]

/// [Redis service sample - HandleRequestThrow]
std::string KeyValue::HandleRequestThrow(
    const server::http::HttpRequest& request,
    server::request::RequestContext& /*context*/) const {
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
/// [Redis service sample - HandleRequestThrow]

/// [Redis service sample - GetValue]
std::string KeyValue::GetValue(std::string_view key,
                               const server::http::HttpRequest& request) const {
  const auto result = redis_client_->Get(std::string{key}, redis_cc_).Get();
  if (!result) {
    request.SetResponseStatus(server::http::HttpStatus::kNotFound);
    return {};
  }
  return *result;
}
/// [Redis service sample - GetValue]

/// [Redis service sample - PostValue]
std::string KeyValue::PostValue(
    std::string_view key, const server::http::HttpRequest& request) const {
  const auto& value = request.GetArg("value");
  const auto result =
      redis_client_->SetIfNotExist(std::string{key}, value, redis_cc_).Get();
  if (!result) {
    request.SetResponseStatus(server::http::HttpStatus::kConflict);
    return {};
  }

  request.SetResponseStatus(server::http::HttpStatus::kCreated);
  return std::string{value};
}
/// [Redis service sample - PostValue]

/// [Redis service sample - DeleteValue]
std::string KeyValue::DeleteValue(std::string_view key) const {
  const auto result = redis_client_->Del(std::string{key}, redis_cc_).Get();
  return std::to_string(result);
}
/// [Redis service sample - DeleteValue]

EvalSha::EvalSha(const components::ComponentConfig& config,
                 const components::ComponentContext& context)
    : server::handlers::HttpHandlerBase(config, context),
      redis_client_{
          context.FindComponent<components::Redis>("key-value-database")
              .GetClient("taxi-tmp")} {}

std::string EvalSha::HandleRequestThrow(
    const server::http::HttpRequest& request,
    server::request::RequestContext&) const {
  const auto& command = request.GetArg("command");
  if (command.empty()) {
    throw server::handlers::ClientError(
        server::handlers::ExternalBody{"No 'command' query argument"});
  }

  if (command == "evalsha") {
    return EvalShaRequest(request);
  }
  if (command == "scriptload") {
    return ScriptLoad(request);
  }
  throw server::handlers::ClientError(server::handlers::ExternalBody{
      "Invalid 'command' query argument: must be 'evalsha' or 'scriptload'"});
}

std::string EvalSha::EvalShaRequest(
    const server::http::HttpRequest& request) const {
  const auto script_hash = request.GetArg("hash");
  if (script_hash.empty()) {
    throw server::handlers::ClientError(
        server::handlers::ExternalBody{"No 'hash' query argument"});
  }
  const auto& key = request.GetArg("key");
  if (key.empty()) {
    throw server::handlers::ClientError(
        server::handlers::ExternalBody{"No 'key' query argument"});
  }

  auto redis_request =
      redis_client_->EvalSha<std::string>(script_hash, {key}, {}, redis_cc_);
  const auto eval_sha_result = redis_request.Get();
  if (eval_sha_result.HasValue()) {
    return eval_sha_result.Get();
  }
  if (eval_sha_result.IsNoScriptError()) {
    return "NOSCRIPT";
  }

  throw server::handlers::InternalServerError(
      server::handlers::ExternalBody{"Some internal redis error"});
}

std::string EvalSha::ScriptLoad(
    const server::http::HttpRequest& request) const {
  const auto script = request.GetArg("script");
  if (script.empty()) {
    throw server::handlers::ClientError(
        server::handlers::ExternalBody{"No 'script' query argument"});
  }
  const auto& key = request.GetArg("key");
  if (key.empty()) {
    throw server::handlers::ClientError(
        server::handlers::ExternalBody{"No 'key' query argument"});
  }

  const auto shard = redis_client_->ShardByKey(key);
  auto redis_request = redis_client_->ScriptLoad(script, shard, redis_cc_);
  /// Return script hash
  return redis_request.Get();
}
}  // namespace samples::redis

/// [Redis service sample - main]
int main(int argc, char* argv[]) {
  const auto component_list =
      components::MinimalServerComponentList()
          .Append<samples::redis::KeyValue>()
          .Append<samples::redis::EvalSha>()
          .Append<components::Secdist>()
          .Append<components::DefaultSecdistProvider>()
          .Append<components::Redis>("key-value-database")
          .Append<components::TestsuiteSupport>()
          .Append<clients::dns::Component>();
  return utils::DaemonMain(argc, argv, component_list);
}
/// [Redis service sample - main]
