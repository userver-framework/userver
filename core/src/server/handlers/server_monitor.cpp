#include <userver/server/handlers/server_monitor.hpp>

#include <userver/components/component.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/server/handlers/exceptions.hpp>
#include <userver/utils/statistics/graphite.hpp>
#include <userver/utils/statistics/json.hpp>
#include <userver/utils/statistics/pretty_format.hpp>
#include <userver/utils/statistics/prometheus.hpp>
#include <userver/utils/statistics/solomon.hpp>
#include <userver/utils/statistics/storage.hpp>
#include <userver/utils/trivial_map.hpp>
#include <userver/yaml_config/merge_schemas.hpp>
#include <userver/yaml_config/schema.hpp>

#include <utils/statistics/value_builder_helpers.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers {

enum class impl::StatsFormat {
  kInternal,
  kGraphite,
  kPrometheus,
  kPrometheusUntyped,
  kJson,
  kPretty,
  kSolomon,
};

namespace {

using impl::StatsFormat;

std::optional<StatsFormat> ParseFormat(std::string_view format) {
  if (format.empty()) return {};

  constexpr utils::TrivialBiMap kToFormat = [](auto selector) {
    return selector()
        .Case("graphite", StatsFormat::kGraphite)
        .Case("prometheus", StatsFormat::kPrometheus)
        .Case("prometheus-untyped", StatsFormat::kPrometheusUntyped)
        .Case("json", StatsFormat::kJson)
        .Case("pretty", StatsFormat::kPretty)
        .Case("solomon", StatsFormat::kSolomon)
        .Case("internal", StatsFormat::kInternal);
  };

  const auto opt_value = kToFormat.TryFind(format);
  if (opt_value.has_value()) {
    return opt_value;
  }
  throw handlers::ClientError(handlers::ExternalBody{
      fmt::format("Unknown format value '{}'. Expected one "
                  "of the following formats: {}",
                  format, kToFormat.DescribeFirst())});
}

}  // namespace

ServerMonitor::ServerMonitor(
    const components::ComponentConfig& config,
    const components::ComponentContext& component_context)
    : HttpHandlerBase(config, component_context, /*is_monitor = */ true),
      statistics_storage_(
          component_context.FindComponent<components::StatisticsStorage>()
              .GetStorage()),
      common_labels_{config["common-labels"].As<CommonLabels>({})},
      default_format_{ParseFormat(config["format"].As<std::string>({}))} {}

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

  const auto arg_format = ParseFormat(request.GetArg("format"));

  if (!default_format_.has_value() && !arg_format.has_value()) {
    throw handlers::ClientError(
        handlers::ExternalBody{"No format was provided"});
  }

  const auto format =
      arg_format.has_value() ? arg_format.value() : default_format_.value();

  using utils::statistics::Request;
  auto common_labels =
      format == StatsFormat::kSolomon ? Request::AddLabels{} : common_labels_;
  const auto statistics_request =
      (path.empty() ? Request::MakeWithPrefix(prefix, std::move(common_labels),
                                              std::move(labels))
                    : Request::MakeWithPath(path, std::move(common_labels),
                                            std::move(labels)));

  request.GetHttpResponse().SetContentType("text/plain; charset=utf-8");
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
      request.GetHttpResponse().SetContentType("application/json");
      return utils::statistics::ToJsonFormat(statistics_storage_,
                                             statistics_request);

    case StatsFormat::kPretty:
      return utils::statistics::ToPrettyFormat(statistics_storage_,
                                               statistics_request);

    case StatsFormat::kSolomon:
      request.GetHttpResponse().SetContentType("application/json");
      return utils::statistics::ToSolomonFormat(
          statistics_storage_, common_labels_, statistics_request);

    case StatsFormat::kInternal:
      request.GetHttpResponse().SetContentType("application/json");
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
    format:
        type: string
        description: Default metrics format. Either static option or URL parameter has to be provided.
        enum:
          - graphite
          - prometheus
          - prometheus-untyped
          - json
          - pretty
          - solomon
          - internal
  )");
}

}  // namespace server::handlers

USERVER_NAMESPACE_END
