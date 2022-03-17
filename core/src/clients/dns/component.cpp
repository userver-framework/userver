#include <userver/clients/dns/component.hpp>

#include <userver/clients/dns/config.hpp>
#include <userver/components/component.hpp>
#include <userver/formats/json/inline.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/utils/statistics/metadata.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::dns {
namespace {

ResolverConfig ParseResolverConfig(
    const components::ComponentConfig& component_config) {
  ResolverConfig config;
  config.file_path =
      component_config["hosts-file-path"].As<std::string>(config.file_path);
  config.file_update_interval =
      component_config["hosts-file-update-interval"]
          .As<std::chrono::milliseconds>(config.file_update_interval);
  config.network_timeout =
      component_config["network-timeout"].As<std::chrono::milliseconds>(
          config.network_timeout);
  config.network_attempts =
      component_config["network-attempts"].As<int>(config.network_attempts);
  config.network_custom_servers =
      component_config["network-custom-servers"].As<std::vector<std::string>>(
          config.network_custom_servers);
  config.cache_ways =
      component_config["cache-ways"].As<size_t>(config.cache_ways);
  config.cache_size_per_way = component_config["cache_size_per_way"].As<size_t>(
      config.cache_size_per_way);
  config.cache_max_reply_ttl =
      component_config["cache_max_reply_ttl"].As<std::chrono::milliseconds>(
          config.cache_max_reply_ttl);
  config.cache_failure_ttl =
      component_config["cache_failure_ttl"].As<std::chrono::milliseconds>(
          config.cache_failure_ttl);
  return config;
}

}  // namespace

Component::Component(const components::ComponentConfig& config,
                     const components::ComponentContext& context)
    : LoggableComponentBase{config, context},
      resolver_{context.GetTaskProcessor(
                    config["fs-task-processor"].As<std::string>()),
                ParseResolverConfig(config)} {
  auto& storage =
      context.FindComponent<components::StatisticsStorage>().GetStorage();
  statistics_holder_ = storage.RegisterExtender(
      config.Name(),
      [this](const auto& /*request*/) { return ExtendStatistics(); });
}

clients::dns::Resolver& Component::GetResolver() { return resolver_; }

formats::json::Value Component::ExtendStatistics() {
  const auto& counters = GetResolver().GetLookupSourceCounters();
  formats::json::ValueBuilder json_counters;
  json_counters["file"] = counters.file.Load();
  json_counters["cached"] = counters.cached.Load();
  json_counters["cached-stale"] = counters.cached_stale.Load();
  json_counters["cached-failure"] = counters.cached_failure.Load();
  json_counters["network"] = counters.network.Load();
  json_counters["network-failure"] = counters.network_failure.Load();
  utils::statistics::SolomonChildrenAreLabelValues(json_counters,
                                                   "dns_reply_source");
  return formats::json::MakeObject("replies", json_counters.ExtractValue());
}

yaml_config::Schema Component::GetStaticConfigSchema() {
  return yaml_config::Schema(R"(
type: object
description: dns-client config
additionalProperties: false
properties:
    fs-task-processor:
        type: string
        description: task processor for disk I/O operations
    hosts-file-path:
        type: string
        description: path to the `hosts` file
        defaultDescription: /etc/hosts
    hosts-file-update-interval:
        type: string
        description: "`hosts` file cache reload interval"
        defaultDescription: 5m
    network-timeout:
        type: string
        description: timeout for network requests
        defaultDescription: 1s
    network-attempts:
        type: integer
        description: number of attempts for network requests
        defaultDescription: 1
    network-custom-servers:
        type: array
        description: list of name servers to use
        defaultDescription: from `/etc/resolv.conf`
        items:
            type: string
            description: name server to use
    cache-ways:
        type: integer
        description: number of ways for network cache
        defaultDescription: 16
    cache-size-per-way:
        type: integer
        description: size of each way of network cache
        defaultDescription: 256
    cache-max-reply-ttl:
        type: string
        description: TTL limit for network replies caching
        defaultDescription: 5m
    cache-failure-ttl:
        type: string
        description: TTL for network failures caching
        defaultDescription: 5s
)");
}

}  // namespace clients::dns

USERVER_NAMESPACE_END
