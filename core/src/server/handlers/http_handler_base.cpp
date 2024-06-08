#include <userver/server/handlers/http_handler_base.hpp>

#include <fmt/core.h>
#include <boost/algorithm/string/split.hpp>
#include <boost/container/small_vector.hpp>

#include <server/handlers/http_handler_base_statistics.hpp>
#include <server/http/http_request_impl.hpp>
#include <server/middlewares/handler_adapter.hpp>
#include <server/request/internal_request_context.hpp>
#include <server/server_config.hpp>

#include <userver/components/component.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/dynamic_config/storage/component.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/engine/task/inherited_variable.hpp>
#include <userver/hostinfo/blocking/get_hostname.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/logging/level_serialization.hpp>
#include <userver/logging/log.hpp>
#include <userver/server/component.hpp>
#include <userver/server/handlers/impl/deadline_propagation_config.hpp>
#include <userver/server/http/http_error.hpp>
#include <userver/server/http/http_method.hpp>
#include <userver/server/http/http_response_body_stream.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/server/middlewares/configuration.hpp>
#include <userver/server/middlewares/http_middleware_base.hpp>
#include <userver/tracing/tracing.hpp>
#include <userver/utils/algo.hpp>
#include <userver/utils/fast_scope_guard.hpp>
#include <userver/utils/from_string.hpp>
#include <userver/utils/graphite.hpp>
#include <userver/utils/log.hpp>
#include <userver/utils/overloaded.hpp>
#include <userver/utils/scope_guard.hpp>
#include <userver/utils/text_light.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers {
namespace {

const std::string kHostname = hostinfo::blocking::GetRealHostName();

// "request" is redundant: https://st.yandex-team.ru/TAXICOMMON-1793
// set to 1 if you need server metrics
constexpr bool kIncludeServerHttpMetrics = false;

std::vector<http::HttpMethod> InitAllowedMethods(const HandlerConfig& config) {
  std::vector<http::HttpMethod> allowed_methods;
  std::vector<std::string> methods;
  boost::split(methods, config.method, [](char c) { return c == ','; });
  for (const auto& method_str : methods) {
    auto method = http::HttpMethodFromString(method_str);
    if (!http::IsHandlerMethod(method)) {
      throw std::runtime_error(method_str + " is not supported in method list");
    }
    allowed_methods.push_back(method);
  }
  return allowed_methods;
}

void SetFormattedErrorResponse(http::HttpResponse& http_response,
                               FormattedErrorData&& formatted_error_data) {
  http_response.SetData(std::move(formatted_error_data.external_body));
  if (formatted_error_data.content_type) {
    http_response.SetContentType(*std::move(formatted_error_data.content_type));
  }
}

void SetFormattedErrorResponse(
    server::http::ResponseBodyStream& response_body_stream,
    FormattedErrorData&& formatted_error, engine::Deadline deadline) {
  if (formatted_error.content_type) {
    response_body_stream.SetHeader(
        USERVER_NAMESPACE::http::headers::kContentType,
        std::move(formatted_error.content_type->ToString()));
  }
  response_body_stream.SetEndOfHeaders();
  response_body_stream.PushBodyChunk(std::move(formatted_error.external_body),
                                     deadline);
}

std::unordered_map<int, logging::Level> ParseStatusCodesLogLevel(
    const std::unordered_map<std::string, std::string>& codes) {
  std::unordered_map<int, logging::Level> result;
  for (const auto& [key, value] : codes) {
    result[utils::FromString<int>(key)] = logging::LevelFromString(value);
  }
  return result;
}

constexpr std::string_view kMiddlePipelineBuilderKey{"pipeline-builder"};

void ValidateMiddlewaresConfiguration(
    const yaml_config::YamlConfig& middlewares_config,
    const middlewares::MiddlewaresList& middleware_names) {
  std::unordered_set<std::string> unique_names;
  unique_names.reserve(middleware_names.size());

  for (const auto& middleware_name : middleware_names) {
    const auto [_, inserted] = unique_names.emplace(middleware_name);
    if (!inserted) {
      throw std::runtime_error{fmt::format(
          "Middleware '{}' is present more than once in the pipeline",
          middleware_name)};
    }
  }

  if (middlewares_config.IsMissing()) {
    return;
  }
  middlewares_config.CheckObjectOrNull();

  for (const auto& [name, _] : yaml_config::Items(middlewares_config)) {
    if (name == kMiddlePipelineBuilderKey) {
      continue;
    }

    if (!unique_names.count(name)) {
      throw std::runtime_error{
          fmt::format("There is a configuration for '{}', but such middleware "
                      "is not present in the pipeline",
                      name)};
    }
  }
}

}  // namespace

HttpHandlerBase::HttpHandlerBase(const components::ComponentConfig& config,
                                 const components::ComponentContext& context,
                                 bool is_monitor)
    : HandlerBase(config, context, is_monitor),
      config_source_(
          context.FindComponent<components::DynamicConfig>().GetSource()),
      allowed_methods_(InitAllowedMethods(GetConfig())),
      handler_name_(config.Name()),
      log_level_(config["log-level"].As<std::optional<logging::Level>>()),
      log_level_for_status_codes_(ParseStatusCodesLogLevel(
          config["status-codes-log-level"]
              .As<std::unordered_map<std::string, std::string>>({}))),
      handler_statistics_(std::make_unique<HttpHandlerStatistics>()),
      request_statistics_(std::make_unique<HttpRequestStatistics>()),
      is_body_streamed_(config["response-body-stream"].As<bool>(false)) {
  if (allowed_methods_.empty()) {
    LOG_WARNING() << "empty allowed methods list in " << config.Name();
  }

  auto& server_component = context.FindComponent<components::Server>();

  engine::TaskProcessor& task_processor =
      context.GetTaskProcessor(GetConfig().task_processor);
  try {
    server_component.AddHandler(*this, task_processor);
  } catch (const std::exception& ex) {
    throw std::runtime_error(std::string("can't add handler to server: ") +
                             ex.what());
  }

  std::vector<utils::statistics::Label> labels{
      {"http_handler", config.Name()},
  };

  auto prefix = std::visit(
      utils::Overloaded{[&](const std::string& path) {
                          labels.emplace_back(
                              "http_path", utils::graphite::EscapeName(path));
                          return std::string{"http"};
                        },
                        [](FallbackHandler fallback) {
                          return "http.by-fallback." + ToString(fallback);
                        }},
      GetConfig().path);

  BuildMiddlewarePipeline(config, context);

  auto& statistics_storage =
      context.FindComponent<components::StatisticsStorage>().GetStorage();
  statistics_holder_ = statistics_storage.RegisterWriter(
      std::move(prefix),
      [this](utils::statistics::Writer& result) {
        FormatStatistics(result["handler"], *handler_statistics_);
        if constexpr (kIncludeServerHttpMetrics) {
          FormatStatistics(result["request"], *request_statistics_);
        }
      },
      std::move(labels));

  set_response_server_hostname_ =
      GetConfig().set_response_server_hostname.value_or(
          server_component.GetServer()
              .GetConfig()
              .set_response_server_hostname);
}

HttpHandlerBase::~HttpHandlerBase() { statistics_holder_.Unregister(); }

void HttpHandlerBase::HandleRequestStream(
    const http::HttpRequest& http_request,
    request::RequestContext& context) const {
  auto& response = http_request.GetHttpResponse();
  const utils::ScopeGuard scope([&response] { response.SetHeadersEnd(); });

  auto& http_response = http_request.GetHttpResponse();
  server::http::ResponseBodyStream response_body_stream{
      response.GetBodyProducer(), http_response};

  // Just in case HandleStreamRequest() throws an exception.
  // Though it can be changed in HandleStreamRequest().
  response_body_stream.SetStatusCode(500);

  try {
    HandleStreamRequest(http_request, context, response_body_stream);
  } catch (const CustomHandlerException& e) {
    response_body_stream.SetStatusCode(http::GetHttpStatus(e));

    for (const auto& [name, value] : e.GetExtraHeaders()) {
      response_body_stream.SetHeader(name, value);
    }
    if (e.IsExternalErrorBodyFormatted()) {
      response_body_stream.SetEndOfHeaders();
      response_body_stream.PushBodyChunk(std::string{e.GetExternalErrorBody()},
                                         engine::Deadline());
    } else {
      auto formatted_error = GetFormattedExternalErrorBody(e);
      SetFormattedErrorResponse(response_body_stream,
                                std::move(formatted_error), engine::Deadline());
    }
  } catch (const std::exception& e) {
    if (engine::current_task::ShouldCancel()) {
      LOG_WARNING() << "request task cancelled, exception in '" << HandlerName()
                    << "' handler in handle_request: " << e;
      response_body_stream.SetStatusCode(
          http::HttpStatus::kClientClosedRequest);
    } else {
      LOG_ERROR() << "exception in '" << HandlerName()
                  << "' handler in handle_request: " << e;
      response_body_stream.SetStatusCode(500);
      SetFormattedErrorResponse(response,
                                GetFormattedExternalErrorBody({
                                    HandlerErrorCode::kServerSideError,
                                    ExternalBody{response.GetData()},
                                }));
    }
  }
}

void HttpHandlerBase::HandleHttpRequest(
    http::HttpRequest& request, request::RequestContext& context) const {
  auto& http_request = request;
  auto& response = http_request.GetHttpResponse();

  // Don't hold the config snapshot for too long, especially with streaming.
  context.GetInternalContext().ResetConfigSnapshot();

  const auto scope_time =
      tracing::ScopeTime::CreateOptionalScopeTime("http_handle_request");
  if (response.IsBodyStreamed()) {
    HandleRequestStream(http_request, context);
  } else {
    // !IsBodyStreamed()
    response.SetData(HandleRequestThrow(http_request, context));
  }
}

void HttpHandlerBase::HandleRequest(request::RequestBase& request,
                                    request::RequestContext& context) const {
  UASSERT(dynamic_cast<http::HttpRequestImpl*>(&request));
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
  auto& http_request_impl = static_cast<http::HttpRequestImpl&>(request);
  http::HttpRequest http_request(http_request_impl);
  auto& response = http_request.GetHttpResponse();

  context.GetInternalContext().SetConfigSnapshot(config_source_.GetSnapshot());
  try {
    UASSERT(first_middleware_);
    first_middleware_->HandleRequest(http_request, context);
  } catch (const std::exception& ex) {
    UASSERT_MSG(false,
                "Middlewares should handle exceptions by themselves and not "
                "let them propagate to the handler");
    LOG_ERROR() << "unable to handle request: " << ex;
    response.SetStatus(http::HttpStatus::kInternalServerError);
  }

  SetResponseServerHostname(response);
  response.SetHeadersEnd();
}

void HttpHandlerBase::ThrowUnsupportedHttpMethod(
    const http::HttpRequest& request) const {
  throw ClientError(
      HandlerErrorCode::kInvalidUsage,
      InternalMessage{fmt::format("method {} is not allowed in {}",
                                  request.GetMethodStr(), HandlerName())});
}

std::string HttpHandlerBase::HandleRequestThrow(
    const http::HttpRequest&, request::RequestContext&) const {
  throw std::runtime_error(
      "non-stream HandleRequestThrow() is executed, but the handler doesn't "
      "override HandleRequestThrow().");
}

void HttpHandlerBase::HandleStreamRequest(
    const server::http::HttpRequest&, server::request::RequestContext&,
    server::http::ResponseBodyStream&) const {
  throw std::runtime_error(
      "stream HandleStreamRequest() is executed, but the handler doesn't "
      "override HandleStreamRequest().");
}

void HttpHandlerBase::ReportMalformedRequest(
    request::RequestBase& request) const {
  try {
    UASSERT(dynamic_cast<http::HttpRequestImpl*>(&request));
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
    auto& http_request_impl = static_cast<http::HttpRequestImpl&>(request);
    const http::HttpRequest http_request(http_request_impl);
    auto& response = http_request.GetHttpResponse();

    SetFormattedErrorResponse(
        response,
        GetFormattedExternalErrorBody({HandlerErrorCode::kRequestParseError,
                                       ExternalBody{response.GetData()}}));
  } catch (const std::exception& ex) {
    LOG_ERROR() << "unable to handle ready request: " << ex;
  }
}

const std::string& HttpHandlerBase::HandlerName() const {
  return handler_name_;
}

const std::vector<http::HttpMethod>& HttpHandlerBase::GetAllowedMethods()
    const {
  return allowed_methods_;
}

HttpHandlerStatistics& HttpHandlerBase::GetHandlerStatistics() const {
  return *handler_statistics_;
}

HttpRequestStatistics& HttpHandlerBase::GetRequestStatistics() const {
  return *request_statistics_;
}

logging::Level HttpHandlerBase::GetLogLevelForResponseStatus(
    http::HttpStatus status) const {
  const auto status_code = static_cast<int>(status);
  const auto* const level =
      utils::FindOrNullptr(log_level_for_status_codes_, status_code);
  if (level) return *level;

  if (status_code >= 400 && status_code <= 499) return logging::Level::kWarning;
  if (status_code >= 500 && status_code <= 599) return logging::Level::kError;
  return logging::Level::kInfo;
}

FormattedErrorData HttpHandlerBase::GetFormattedExternalErrorBody(
    const CustomHandlerException& exc) const {
  return {exc.GetExternalErrorBody()};
}

std::string HttpHandlerBase::GetRequestBodyForLogging(
    const http::HttpRequest&, request::RequestContext&,
    const std::string& request_body) const {
  const std::size_t limit = GetConfig().request_body_size_log_limit;
  return utils::log::ToLimitedUtf8(request_body, limit);
}

std::string HttpHandlerBase::GetResponseDataForLogging(
    const http::HttpRequest&, request::RequestContext&,
    const std::string& response_data) const {
  const std::size_t limit = GetConfig().response_data_size_log_limit;
  return utils::log::ToLimitedUtf8(response_data, limit);
}

std::string HttpHandlerBase::GetMetaType(
    const http::HttpRequest& request) const {
  return request.GetRequestPath();
}

std::string HttpHandlerBase::GetRequestBodyForLoggingChecked(
    const http::HttpRequest& request, request::RequestContext& context,
    const std::string& request_body) const {
  try {
    return GetRequestBodyForLogging(request, context, request_body);
  } catch (const std::exception& ex) {
    LOG_LIMITED_ERROR() << "failed to get request body for logging: " << ex;
    return "<error in GetRequestBodyForLogging>";
  }
}

std::string HttpHandlerBase::GetResponseDataForLoggingChecked(
    const http::HttpRequest& request, request::RequestContext& context,
    const std::string& response_data) const {
  try {
    return GetResponseDataForLogging(request, context, response_data);
  } catch (const std::exception& ex) {
    LOG_LIMITED_ERROR() << "failed to get response data for logging: " << ex;
    return "<error in GetResponseDataForLogging>";
  }
}

void HttpHandlerBase::HandleCustomHandlerException(
    const http::HttpRequest& request, const CustomHandlerException& ex) const {
  auto http_status = http::GetHttpStatus(ex);
  const auto level = GetLogLevelForResponseStatus(http_status);
  LOG(level) << "custom handler exception in '" << HandlerName()
             << "' handler: msg=" << ex;

  auto& response = request.GetHttpResponse();
  response.SetStatus(http_status);
  if (ex.IsExternalErrorBodyFormatted()) {
    response.SetData(ex.GetExternalErrorBody());
  } else {
    SetFormattedErrorResponse(response, GetFormattedExternalErrorBody(ex));
  }
  for (const auto& [name, value] : ex.GetExtraHeaders()) {
    response.SetHeader(name, value);
  }
}

void HttpHandlerBase::HandleUnknownException(const http::HttpRequest& request,
                                             const std::exception& ex) const {
  LogUnknownException(ex);

  auto& response = request.GetHttpResponse();
  if (engine::current_task::ShouldCancel()) {
    response.SetStatus(http::HttpStatus::kClientClosedRequest);
  } else {
    request.impl_.MarkAsInternalServerError();
    SetFormattedErrorResponse(response,
                              GetFormattedExternalErrorBody({
                                  handlers::HandlerErrorCode::kServerSideError,
                                  handlers::ExternalBody{},
                              }));
  }
}

void HttpHandlerBase::LogUnknownException(const std::exception& ex) const {
  if (engine::current_task::ShouldCancel()) {
    LOG_WARNING() << "request task cancelled, exception in '" << HandlerName()
                  << "' handler: " << ex;
  } else {
    LOG_ERROR() << "exception in '" << HandlerName() << "' handler: " << ex;
  }
}

const std::optional<logging::Level>& HttpHandlerBase::GetLogLevel() const {
  return log_level_;
}

template <typename HttpStatistics>
void HttpHandlerBase::FormatStatistics(utils::statistics::Writer result,
                                       const HttpStatistics& stats) {
  using Snapshot = typename HttpStatistics::Snapshot;
  Snapshot total;

  for (const auto method : GetAllowedMethods()) {
    const Snapshot by_method{stats.GetByMethod(method)};
    if (IsMethodStatisticIncluded()) {
      result.ValueWithLabels(by_method, {"http_method", ToString(method)});
    }
    total.Add(by_method);
  }

  result = total;
}

void HttpHandlerBase::SetResponseServerHostname(
    http::HttpResponse& response) const {
  if (set_response_server_hostname_) {
    response.SetHeader(USERVER_NAMESPACE::http::headers::kXYaTaxiServerHostname,
                       kHostname);
  }
}

void HttpHandlerBase::BuildMiddlewarePipeline(
    const components::ComponentConfig& config,
    const components::ComponentContext& context) {
  if (!context.FindComponentOptional<middlewares::HandlerAdapterFactory>()) {
    // We have everything needed in {Minimal|Common}ServerComponentList, but
    // some people might not use it, give them some diagnostics.
    throw std::runtime_error{
        "It seems that you are building your ComponentList from scratch, "
        "append DefaultMiddlewareComponents() from "
        "userver/server/middlewares/configuration.hpp to it via "
        "AppendComponentList()"};
  }

  const auto middlewares_config = config["middlewares"];

  const auto& handler_pipeline_builder =
      context.FindComponent<middlewares::HandlerPipelineBuilder>(
          middlewares_config[kMiddlePipelineBuilderKey].As<std::string>(
              middlewares::HandlerPipelineBuilder::kName));
  const auto handler_middlewares = handler_pipeline_builder.BuildPipeline(
      context.FindComponent<components::Server>()
          .GetServer()
          .GetCommonMiddlewares());

  ValidateMiddlewaresConfiguration(middlewares_config, handler_middlewares);

  auto* next_middleware_ptr_{&first_middleware_};
  const auto add_middleware = [this, &middlewares_config, &context,
                               &next_middleware_ptr_](std::string_view name) {
    *next_middleware_ptr_ =
        context.FindComponent<middlewares::HttpMiddlewareFactoryBase>(name)
            .CreateChecked(*this, middlewares_config[name]);
    next_middleware_ptr_ = &(*next_middleware_ptr_)->next_;
  };

  for (const auto& middleware_name : handler_middlewares) {
    add_middleware(middleware_name);
  }

  // Finalize the pipeline
  { add_middleware(middlewares::HandlerAdapterFactory::kName); }
}

yaml_config::Schema HttpHandlerBase::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<HandlerBase>(R"(
type: object
description: Base class for all the userver HTTP handlers
additionalProperties: false
properties:
    log-level:
        type: string
        description: overrides log level for this handle
        defaultDescription: <no override>
    status-codes-log-level:
        type: object
        properties: {}
        additionalProperties:
            type: string
            description: log level
        description: HTTP status code -> log level map
    middlewares:
        type: object
        properties:
            pipeline-builder:
                type: string
                description: name of a component to build a middleware pipeline for this particular handler
                defaultDescription: default-handler-middleware-pipeline-builder
        additionalProperties:
            type: object
            properties: {}
            additionalProperties: true
            description: per-middleware configuration
        description: middleware name -> middleware configuration map
)");
}

}  // namespace server::handlers

USERVER_NAMESPACE_END
