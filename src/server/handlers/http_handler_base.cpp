#include <server/handlers/http_handler_base.hpp>

#include <json/writer.h>

#include <logging/log.hpp>
#include <server/request/http_server_settings_base_component.hpp>
#include <utils/uuid4.hpp>

#include <server/http/http_request_impl.hpp>

namespace server {
namespace handlers {
namespace {

const std::string kLink = "link";
const std::string kXYaRequestId = "X-YaRequestId";

template <typename HeadersHolder>
std::string GetHeadersLogString(const HeadersHolder& headers_holder) {
  Json::Value json_headers(Json::objectValue);
  for (const auto& header_name : headers_holder.GetHeaderNames()) {
    json_headers[header_name] = headers_holder.GetHeader(header_name);
  }
  Json::FastWriter json_writer;
  json_writer.omitEndingLineFeed();
  return json_writer.write(json_headers);
}

}  // namespace

HttpHandlerBase::HttpHandlerBase(
    const components::ComponentConfig& config,
    const components::ComponentContext& component_context, bool is_monitor)
    : HandlerBase(config, component_context, is_monitor),
      http_server_settings_(
          component_context
              .FindComponent<components::HttpServerSettingsBase>()) {}

void HttpHandlerBase::HandleRequest(const request::RequestBase& request,
                                    request::RequestContext& context) const
    noexcept {
  try {
    const http::HttpRequest http_request(
        dynamic_cast<const http::HttpRequestImpl&>(request));
    auto& response = http_request.GetHttpResponse();

    const std::string& link = utils::generators::GenerateUuid();
    context.GetLogExtra().Extend(kLink, link,
                                 logging::LogExtra::ExtendType::kFrozen);

    bool log_request =
        http_server_settings_ ? http_server_settings_->NeedLogRequest() : false;
    bool log_request_headers =
        http_server_settings_ ? http_server_settings_->NeedLogRequestHeaders()
                              : false;

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

}  // namespace handlers
}  // namespace server
