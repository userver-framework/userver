#include <userver/clients/dns/component.hpp>

#include <userver/clients/dns/config.hpp>
#include <userver/components/component.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/formats/json/inline.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/utils/statistics/metadata.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::dns {
namespace {

constexpr std::string_view kDnsReplySource = "dns_reply_source";

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
  statistics_holder_ = storage.RegisterWriter(
      config.Name() + ".replies", [this](auto& writer) { Write(writer); });
}

clients::dns::Resolver& Component::GetResolver() { return resolver_; }

void Component::Write(utils::statistics::Writer& writer) {
  const auto& counters = GetResolver().GetLookupSourceCounters();
  writer.ValueWithLabels(counters.file, {kDnsReplySource, "file"});
  writer.ValueWithLabels(counters.cached, {kDnsReplySource, "cached"});
  writer.ValueWithLabels(counters.cached_stale,
                         {kDnsReplySource, "cached-stale"});
  writer.ValueWithLabels(counters.cached_failure,
                         {kDnsReplySource, "cached-failure"});
  writer.ValueWithLabels(counters.network, {kDnsReplySource, "network"});
  writer.ValueWithLabels(counters.network_failure,
                         {kDnsReplySource, "network-failure"});
}

yaml_config::Schema Component::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<LoggableComponentBase>(R"(
type: object
description: Caching DNS resolver component.
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
