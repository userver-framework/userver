#include <ugrpc/server/impl/parse_config.hpp>

#include <boost/range/adaptor/transformed.hpp>

#include <userver/logging/component.hpp>
#include <userver/logging/level_serialization.hpp>
#include <userver/logging/null_logger.hpp>
#include <userver/utils/algo.hpp>

#include <userver/ugrpc/server/middlewares/base.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

namespace {

constexpr std::string_view kTaskProcessorKey = "task-processor";
constexpr std::string_view kMiddlewaresKey = "middlewares";

template <typename ParserFunc>
auto ParseOptional(const yaml_config::YamlConfig& service_field,
                   const components::ComponentContext& context,
                   ParserFunc parser_func) {
  using ParseResult = decltype(parser_func(service_field, context));
  if (service_field.IsMissing()) {
    return boost::optional<ParseResult>{};
  }
  return boost::optional<ParseResult>{parser_func(service_field, context)};
}

template <typename Field, typename ParserFunc>
Field MergeField(const yaml_config::YamlConfig& service_field,
                 const boost::optional<Field>& server_default,
                 const components::ComponentContext& context,
                 ParserFunc parser_func) {
  if (service_field.IsMissing() && server_default) {
    return *server_default;
  }
  // Will throw if IsMissing and no server_default and no compile-time default.
  return parser_func(service_field, context);
}

engine::TaskProcessor& ParseTaskProcessor(
    const yaml_config::YamlConfig& field,
    const components::ComponentContext& context) {
  return context.GetTaskProcessor(field.As<std::string>());
}

std::vector<std::string> ParseMiddlewares(
    const yaml_config::YamlConfig& field,
    const components::ComponentContext& /*context*/) {
  return field.As<std::vector<std::string>>();
}

Middlewares FindMiddlewares(const std::vector<std::string>& names,
                            const components::ComponentContext& context) {
  return utils::AsContainer<Middlewares>(
      names | boost::adaptors::transformed([&](const std::string& name) {
        return context.FindComponent<MiddlewareComponentBase>(name)
            .GetMiddleware();
      }));
}

}  // namespace

ServiceDefaults ParseServiceDefaults(
    const yaml_config::YamlConfig& value,
    const components::ComponentContext& context) {
  return ServiceDefaults{
      /*task_processor=*/ParseOptional(value[kTaskProcessorKey], context,
                                       ParseTaskProcessor),
      /*middleware_names=*/
      ParseOptional(value[kMiddlewaresKey], context, ParseMiddlewares),
  };
}

server::ServiceConfig ParseServiceConfig(
    const yaml_config::YamlConfig& value,
    const components::ComponentContext& context,
    const ServiceDefaults& defaults) {
  return server::ServiceConfig{
      /*task_processor=*/MergeField(value[kTaskProcessorKey],
                                    defaults.task_processor, context,
                                    ParseTaskProcessor),
      /*middlewares=*/
      FindMiddlewares(
          MergeField(value[kMiddlewaresKey], defaults.middleware_names, context,
                     ParseMiddlewares),
          context),
  };
}

ServerConfig ParseServerConfig(const yaml_config::YamlConfig& value,
                               const components::ComponentContext& context) {
  ServerConfig config;
  config.unix_socket_path =
      value["unix-socket-path"].As<std::optional<std::string>>();
  config.port = value["port"].As<std::optional<int>>();
  config.completion_queue_num = value["completion-queue-count"].As<int>(2);
  config.channel_args =
      value["channel-args"].As<decltype(config.channel_args)>({});
  config.native_log_level =
      value["native-log-level"].As<logging::Level>(logging::Level::kError);
  config.enable_channelz = value["enable-channelz"].As<bool>(false);

  const auto logger_name = value["access-tskv-logger"];
  if (!logger_name.IsMissing()) {
    config.access_tskv_logger =
        context.FindComponent<components::Logging>().GetLogger(
            logger_name.As<std::string>());
  } else {
    config.access_tskv_logger = logging::MakeNullLogger();
  }

  return config;
}

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
