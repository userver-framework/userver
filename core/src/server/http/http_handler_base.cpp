#include "http_handler_base.hpp"

#include <server/http/http2_request_parser.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/logging/log.hpp>

namespace {

bool findStringIC(const std::string& strHaystack,
                  const std::string& strNeedle) {
  std::basic_string<char>::const_iterator it =
      std::search(strHaystack.begin(), strHaystack.end(), strNeedle.begin(),
                  strNeedle.end(), [](unsigned char ch1, unsigned char ch2) {
                    return std::toupper(ch1) == std::toupper(ch2);
                  });
  return (it != strHaystack.end());
}

}  // namespace

USERVER_NAMESPACE_BEGIN

namespace server::http {

HTTPHandlerBase::HTTPHandlerBase(
    const HandlerInfoIndex& handler_info_index,
    const request::HttpRequestConfig& request_config,
    OnNewRequestCb&& on_new_request_cb, net::ParserStats& stats,
    request::ResponseDataAccounter& data_accounter,
    http::Http2UpgradeData upgrade_data)
    : handler_info_index_(handler_info_index),
      request_constructor_config_(request_config),
      on_new_request_cb_(std::move(on_new_request_cb)),
      stats_(stats),
      data_accounter_(data_accounter),
      upgrade_data_(std::move(upgrade_data)) {
  http1_parser_.emplace(handler_info_index_, request_constructor_config_,
                        server::http::OnNewRequestCb(on_new_request_cb_),
                        stats_, data_accounter_);

  http1_handler_.emplace(*http1_parser_);
}

bool HTTPHandlerBase::Parse(const char* data, size_t size) {
  if (http_protocol_ == HttpProtocol::kHTTP1) {
    bool ret = http1_handler_->Parse(data, size);
    return ret;
  } else {
    return http2_handler_->Parse(data, size);
  }
}

void HTTPHandlerBase::SendResponse(engine::io::Socket& socket,
                                   request::ResponseBase& response) {
  if (http_protocol_ == HttpProtocol::kHTTP1) {
    http1_handler_->SendResponse(socket, response);
  } else {
    if (sendUpgradeData_) {
      http2_handler_->SendResponse(socket, response, upgrade_data_);
      sendUpgradeData_ = false;
    } else {
      http2_handler_->SendResponse(socket, response, {});
    }
  }
}

void HTTPHandlerBase::CheckUpgrade(const http::HttpRequestImpl& http_request) {
  if (http_request.HasUpgradeHeaders() && CheckUpgradeHeaders(http_request)) {
    UpgradeToHttp2(http_request.GetHeader(
        USERVER_NAMESPACE::http::headers::kHttp2Settings));
  }
}

bool HTTPHandlerBase::CheckUpgradeHeaders(
    const http::HttpRequestImpl& http_request) {
  if (!findStringIC(
          http_request.GetHeader(USERVER_NAMESPACE::http::headers::kConnection),
          "upgrade")) {
    return false;
  }

  if (!findStringIC(
          http_request.GetHeader(USERVER_NAMESPACE::http::headers::kUpgrade),
          "h2c")) {
    return false;
  }

  return http_request.HasHeader(
      USERVER_NAMESPACE::http::headers::kHttp2Settings);
}

void HTTPHandlerBase::UpgradeToHttp2(const std::string& settings) {
  LOG_DEBUG() << "Upgrade to HTTP2";
  http_protocol_ = HttpProtocol::kHTTP2;

  http2_parser_.emplace(handler_info_index_, request_constructor_config_,
                        stats_, data_accounter_,
                        server::http::OnNewRequestCb(on_new_request_cb_));

  http2_parser_->ApplySettings(settings, upgrade_data_.server_settings);
  http2_handler_.emplace(*http2_parser_);

  LOG_DEBUG() << "Settings applied";
  sendUpgradeData_ = true;
}

}  // namespace server::http

USERVER_NAMESPACE_END
