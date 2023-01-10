#include <userver/server/handlers/server_monitor.hpp>

#include <userver/components/component.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/server/handlers/exceptions.hpp>
#include <userver/utils/statistics/graphite.hpp>
#include <userver/utils/statistics/json.hpp>
#include <userver/utils/statistics/prometheus.hpp>
#include <userver/utils/statistics/solomon.hpp>
#include <userver/utils/statistics/storage.hpp>
#include <userver/yaml_config/merge_schemas.hpp>
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
  kSolomon,
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
  } else if (format == "solomon") {
    return StatsFormat::kSolomon;
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
              .GetStorage()),
      common_labels_{config["common-labels"].As<CommonLabels>({})} {}

std::string ServerMonitor::HandleRequestThrow(const http::HttpRequest& request,
                                              request::RequestContext&) const {
  const auto& prefix = request.GetArg("prefix");
  const auto& path = request.GetArg("path");
  if (!path.empty() && !prefix.empty() && path != prefix) {
    throw handlers::ClientError(handlers::ExternalBody{
        "Use either 'path' or 'prefix' URL parameter, not both"});
  }

  std::vector<utils::statistics::Label> labels;
  const auto& labels_json = request.GetArg("labels");
  if (!labels_json.empty()) {
    auto json = formats::json::FromString(labels_json);
    for (auto [key, value] : Items(json)) {
      labels.emplace_back(std::move(key), value.As<std::string>());
    }
  }

  const auto format = ParseFormat(request.GetArg("format"));

  using utils::statistics::Request;
  auto common_labels =
      format == StatsFormat::kSolomon ? Request::AddLabels{} : common_labels_;
  const auto statistics_request =
      (path.empty() ? Request::MakeWithPrefix(prefix, std::move(common_labels),
                                              std::move(labels))
                    : Request::MakeWithPath(path, std::move(common_labels),
                                            std::move(labels)));

  switch (format) {
    case StatsFormat::kGraphite:
      return utils::statistics::ToGraphiteFormat(statistics_storage_,
                                                 statistics_request);

    case StatsFormat::kPrometheus:
      return utils::statistics::ToPrometheusFormat(statistics_storage_,
                                                   statistics_request);

    case StatsFormat::kPrometheusUntyped:
      return utils::statistics::ToPrometheusFormatUntyped(statistics_storage_,
                                                          statistics_request);

    case StatsFormat::kJson:
      return utils::statistics::ToJsonFormat(statistics_storage_,
                                             statistics_request);

    case StatsFormat::kSolomon:
      return utils::statistics::ToSolomonFormat(
          statistics_storage_, common_labels_, statistics_request);

    case StatsFormat::kInternal:
      const auto json = statistics_storage_.GetAsJson();
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
  return yaml_config::MergeSchemas<HttpHandlerBase>(R"(
type: object
description: handler-server-monitor config
additionalProperties: false
properties:
    common-labels:
        type: object
        description: |
            A map of label name to label value. Items of the map are
            added to each metric.
        additionalProperties: true
        properties: {}
  )");
}

}  // namespace server::handlers

USERVER_NAMESPACE_END
