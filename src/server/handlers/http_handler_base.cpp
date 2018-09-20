#include <server/handlers/http_handler_base.hpp>

#include <formats/json/serialize.hpp>
#include <formats/json/value_builder.hpp>

#include <logging/log.hpp>
#include <server/handlers/http_handler_base_statistics.hpp>
#include <server/request/http_server_settings_base_component.hpp>
#include <utils/statistics/percentile_json.hpp>
#include <utils/uuid4.hpp>

#include <server/http/http_error.hpp>
#include <server/http/http_request_impl.hpp>
#include <tracing/tracing.hpp>

namespace server {
namespace handlers {
namespace {

const std::string kLink = "link";
const std::string kXYaRequestId = "X-YaRequestId";

template <typename HeadersHolder>
std::string GetHeadersLogString(const HeadersHolder& headers_holder) {
  formats::json::ValueBuilder json_headers(formats::json::Type::kObject);
  for (const auto& header_name : headers_holder.GetHeaderNames()) {
    json_headers[header_name] = headers_holder.GetHeader(header_name);
  }
  return formats::json::ToString(json_headers.ExtractValue());
}

}  // namespace

formats::json::Value HttpHandlerBase::StatisticsToJson(
    const HttpHandlerBase::Statistics& stats) {
  formats::json::ValueBuilder result;
  formats::json::ValueBuilder total;

  formats::json::ValueBuilder reply_codes;
  size_t sum = 0;
  for (auto it : stats.GetReplyCodes()) {
    reply_codes[std::to_string(it.first)] = it.second;
    sum += it.second;
  }
  total["reply-codes"] = std::move(reply_codes);

  total["timings"]["1min"] =
      utils::statistics::PercentileToJson(stats.GetTimings());

  result["total"] = std::move(total);
  return result.ExtractValue();
}

HttpHandlerBase::HttpHandlerBase(
    const components::ComponentConfig& config,
    const components::ComponentContext& component_context, bool is_monitor)
    : HandlerBase(config, component_context, is_monitor),
      http_server_settings_(
          component_context
              .FindComponent<components::HttpServerSettingsBase>()),
      statistics_(std::make_unique<Statistics>()) {}

HttpHandlerBase::~HttpHandlerBase() = default;

void HttpHandlerBase::HandleRequest(const request::RequestBase& request,
                                    request::RequestContext& context) const
    noexcept {
  try {
    const http::HttpRequest http_request(
        dynamic_cast<const http::HttpRequestImpl&>(request));
    auto& response = http_request.GetHttpResponse();
    const auto start_time = std::chrono::system_clock::now();

    const std::string& link = utils::generators::GenerateUuid();
    context.GetLogExtra().Extend(kLink, link,
                                 logging::LogExtra::ExtendType::kFrozen);

    bool log_request =
        http_server_settings_ ? http_server_settings_->NeedLogRequest() : false;
    bool log_request_headers =
        http_server_settings_ ? http_server_settings_->NeedLogRequestHeaders()
                              : false;

    tracing::Span span =
        tracing::Tracer::GetTracer()->CreateSpanWithoutParent("http_request");

    if (log_request) {
      logging::LogExtra log_extra(context.GetLogExtra());

      const auto& parent_link = http_request.GetHeader(kXYaRequestId);
      if (!parent_link.empty()) log_extra.Extend("parent_link", parent_link);

      log_extra.Extend("request_url", http_request.GetUrl());
      if (log_request_headers) {
        log_extra.Extend("request_headers", GetHeadersLogString(http_request));
      }
      log_extra.Extend("request_body", http_request.RequestBody());
      LOG_INFO() << "start handling" << std::move(log_extra);
    }

    try {
      response.SetData(HandleRequestThrow(http_request, context));
    } catch (const http::HttpException& ex) {
      LOG_ERROR() << "http exception in '" << HandlerName()
                  << "' handler in handle_request: code="
                  << HttpStatusString(ex.GetStatus()) << ", msg=" << ex.what()
                  << ", body=" << ex.GetExternalErrorBody();
      response.SetStatus(ex.GetStatus());
      response.SetData(ex.GetExternalErrorBody());
    } catch (const std::exception& ex) {
      LOG_ERROR() << "exception in '" << HandlerName()
                  << "' handler in handle_request: " << ex.what();
      response.SetStatus(server::http::HttpStatus::kInternalServerError);
      response.SetData({});
      response.ClearHeaders();
    }

    response.SetHeader(kXYaRequestId, link);

    if (log_request) {
      logging::LogExtra log_extra(context.GetLogExtra());
      log_extra.Extend("response_code", static_cast<int>(response.GetStatus()));

      if (log_request_headers) {
        log_extra.Extend("response_headers", GetHeadersLogString(response));
      }
      log_extra.Extend("response_data", response.GetData());
      LOG_INFO() << "finish handling " << http_request.GetUrl()
                 << std::move(log_extra);
    }

    const auto finish_time = std::chrono::system_clock::now();
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        finish_time - start_time);
    statistics_->Account(static_cast<int>(response.GetStatus()), ms.count());
  } catch (const std::exception& ex) {
    LOG_ERROR() << "unable to handle request: " << ex.what();
  }
}

void HttpHandlerBase::OnRequestComplete(const request::RequestBase& request,
                                        request::RequestContext& context) const
    noexcept {
  try {
    const http::HttpRequest http_request(
        dynamic_cast<const http::HttpRequestImpl&>(request));
    try {
      OnRequestCompleteThrow(http_request, context);
    } catch (const std::exception& ex) {
      LOG_ERROR() << "exception in '" << HandlerName()
                  << "' hander in on_request_complete: " << ex.what();
    }
  } catch (const std::exception& ex) {
    LOG_ERROR() << "unable to complete request: " << ex.what();
  }
}

formats::json::Value HttpHandlerBase::GetMonitorData(
    components::MonitorVerbosity /*verbosity*/) const {
  return StatisticsToJson(*statistics_);
}

std::string HttpHandlerBase::GetMetricsPath() const {
  // TODO: s/bad symbols/_/g
  return "http-handlers." + GetConfig().path;
}

}  // namespace handlers
}  // namespace server
