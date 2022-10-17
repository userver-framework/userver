#include <userver/server/handlers/server_monitor.hpp>

#include <userver/components/component_context.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/server/handlers/exceptions.hpp>
#include <userver/utils/statistics/graphite.hpp>
#include <userver/utils/statistics/json.hpp>
#include <userver/utils/statistics/prometheus.hpp>
#include <userver/utils/statistics/storage.hpp>
#include <userver/yaml_config/schema.hpp>

#include <utils/statistics/value_builder_helpers.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers {

namespace {

enum class StatsFormat {
  kInternal,
  kGraphite,
  kPrometheus,
  kPrometheusUntyped,
  kJson,
};

StatsFormat ParseFormat(std::string_view format) {
  if (format == "graphite") {
    return StatsFormat::kGraphite;
  } else if (format == "prometheus") {
    return StatsFormat::kPrometheus;
  } else if (format == "prometheus-untyped") {
    return StatsFormat::kPrometheusUntyped;
  } else if (format == "json") {
    return StatsFormat::kJson;
  } else if (format == "internal" || format.empty()) {
    return StatsFormat::kInternal;
  }

  throw handlers::ClientError(
      handlers::ExternalBody{"Unknown value of 'format' URL parameter"});
}

}  // namespace

ServerMonitor::ServerMonitor(
    const components::ComponentConfig& config,
    const components::ComponentContext& component_context)
    : HttpHandlerBase(config, component_context, /*is_monitor = */ true),
      statistics_storage_(
          component_context.FindComponent<components::StatisticsStorage>()
              .GetStorage()) {}

std::string ServerMonitor::HandleRequestThrow(const http::HttpRequest& request,
                                              request::RequestContext&) const {
  utils::statistics::StatisticsRequest statistics_request;
  statistics_request.prefix = request.GetArg("prefix");
  statistics_request.path = request.GetArg("path");
  const auto& labels_json = request.GetArg("labels");
  if (!labels_json.empty()) {
    auto json = formats::json::FromString(labels_json);
    for (auto [key, value] : Items(json)) {
      statistics_request.labels.emplace_back(std::move(key),
                                             value.As<std::string>());
    }
  }

  const auto format = ParseFormat(request.GetArg("format"));
  switch (format) {
    case StatsFormat::kGraphite:
      // TODO: common labels
      return utils::statistics::ToGraphiteFormat({}, statistics_storage_,
                                                 statistics_request);

    case StatsFormat::kPrometheus:
      // TODO: common labels
      return utils::statistics::ToPrometheusFormat({}, statistics_storage_,
                                                   statistics_request);

    case StatsFormat::kPrometheusUntyped:
      // TODO: common labels
      return utils::statistics::ToPrometheusFormatUntyped(
          {}, statistics_storage_, statistics_request);

    case StatsFormat::kJson:
      // TODO: common labels
      return utils::statistics::ToJsonFormat({}, statistics_storage_,
                                             statistics_request);

    case StatsFormat::kInternal:
      const auto json =
          statistics_storage_.GetAsJson(statistics_request).ExtractValue();
      UASSERT(utils::statistics::AreAllMetricsNumbers(json));
      return formats::json::ToString(json);
  }

  UINVARIANT(false, "Unexpected 'format' value");
}

std::string ServerMonitor::GetResponseDataForLogging(const http::HttpRequest&,
                                                     request::RequestContext&,
                                                     const std::string&) const {
  // Useless data for logs, no need to duplicate metrics in logs
  return "<statistics data>";
}

yaml_config::Schema ServerMonitor::GetStaticConfigSchema() {
  auto schema = HttpHandlerBase::GetStaticConfigSchema();
  schema.UpdateDescription("handler-server-monitor config");
  return schema;
}

}  // namespace server::handlers

USERVER_NAMESPACE_END
