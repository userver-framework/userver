#pragma once

#include <fmt/format.h>
#include <userver/clients/dns/config.hpp>
#include <userver/clients/dns/resolver.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/clients/http/streamed_response.hpp>
#include <userver/components/component_context.hpp>
#include <userver/dynamic_config/storage/component.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/http/http_response_body_stream.hpp>
#include <userver/testsuite/testpoint.hpp>
#include <userver/utest/using_namespace_userver.hpp>
#include <userver/utils/assert.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

namespace chaos {

static constexpr std::string_view kSeparator = ", ";
static constexpr size_t kResolverTimeoutSecs = 15;

class ResolverHandler final : public server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-chaos-dns-resolver";

  ResolverHandler(const components::ComponentConfig& config,
                  const components::ComponentContext& context)
      : HttpHandlerBase(config, context),
        resolver_{engine::current_task::GetTaskProcessor(), [&config] {
                    clients::dns::ResolverConfig resolver_config;
                    resolver_config.network_custom_servers = {
                        config["dns-server"].As<std::string>()};
                    resolver_config.file_path =
                        config["hosts-file"].As<std::string>();
                    resolver_config.file_update_interval =
                        std::chrono::seconds{kResolverTimeoutSecs};
                    resolver_config.network_timeout =
                        std::chrono::seconds{kResolverTimeoutSecs};
                    resolver_config.network_attempts = 1;
                    resolver_config.cache_max_reply_ttl = std::chrono::seconds{
                        config["cache-max-ttl"].As<int64_t>()};
                    resolver_config.cache_failure_ttl = std::chrono::seconds{
                        config["cache-failure-ttl"].As<int64_t>()};
                    resolver_config.cache_ways = 1;
                    resolver_config.cache_size_per_way =
                        config["cache-size-per-way"].As<int64_t>();
                    return resolver_config;
                  }()} {
    LOG_DEBUG() << "Connect to dns-server "
                << config["dns-server"].As<std::string>();
  }

  std::string HandleRequestThrow(
      const server::http::HttpRequest& request,
      server::request::RequestContext&) const override {
    const auto& type = request.GetArg("type");
    const auto& timeout = request.GetArg("timeout");
    const std::chrono::seconds timeout_secs{
        timeout.empty() ? kResolverTimeoutSecs : std::stoi(timeout)};
    const auto& to_resolve = request.GetArg("host_to_resolve");
    LOG_DEBUG() << "Timeout ms " << timeout_secs << " but should be "
                << timeout;
    if (type == "resolve") {
      auto res = resolver_.Resolve(
          to_resolve, engine::Deadline::FromDuration(timeout_secs));
      return fmt::to_string(fmt::join(res, kSeparator));
    } else if (type == "flush") {
      resolver_.FlushNetworkCache(to_resolve);
      return "flushed";
    }

    UASSERT(false);
    return {};
  }

  static yaml_config::Schema GetStaticConfigSchema() {
    return yaml_config::MergeSchemas<server::handlers::HttpHandlerBase>(R"(
    type: object
    description: Handler for clients::dns::Resolver testing
    additionalProperties: false
    properties:
      dns-server:
          type: string
          description: address of for mocked dns-server
      hosts-file:
          type: string
          description: hosts file instead of /etc/hosts
      cache-max-ttl:
          type: integer
          description: lifetime in milliseconds of cached results
          defaultDescription: 1000
      cache-failure-ttl:
          type: integer
          description: failure lifetime
          defaultDescription: 0
      cache-size-per-way:
          type: integer
          description: size of cache for hosts
          defaultDescription: 1000
  )");
  }

 private:
  mutable clients::dns::Resolver resolver_;
};

}  // namespace chaos
