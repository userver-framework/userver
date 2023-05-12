#pragma once

#include <nghttp2/nghttp2.h>

#include <server/net/stats.hpp>
#include <server/request/request_parser.hpp>

#include <userver/server/request/request_base.hpp>
#include <userver/server/request/request_config.hpp>

#include "handler_info_index.hpp"
#include "http2_session_data.hpp"

USERVER_NAMESPACE_BEGIN

namespace server::http {

// A lot of copy-pase from http_reqeust_parser.
// TODO Make base class for HttpRequestParser and Http2RequestParser
// with common methods

class Http2RequestParser final : public request::RequestParser {
 public:
  Http2RequestParser(const HandlerInfoIndex&, const request::HttpRequestConfig&,
                     net::ParserStats&, request::ResponseDataAccounter&,
                     OnNewRequestCb&&);

  Http2RequestParser(Http2RequestParser&&) = delete;
  Http2RequestParser& operator=(Http2RequestParser&&) = delete;

  bool Parse(const char* data, size_t size) override;

  bool ApplySettings(const std::string_view,
                     const std::vector<nghttp2_settings_entry>&);

  Http2SessionData* SessionData() { return session_data_.get(); }

 private:
  // const HandlerInfoIndex& handler_info_index_;
  // const request::HttpRequestConfig& request_constructor_config_;

  // std::optional<HttpRequestConstructor> request_constructor_;

  // static const http_parser_settings parser_settings;
  // request::ResponseDataAccounter& data_accounter_;
  // std::unique_ptr<nghttp2_session_callbacks> callbacks_{nullptr};

  net::ParserStats& stats_;
  std::unique_ptr<Http2SessionData> session_data_{nullptr};
};

}  // namespace server::http

USERVER_NAMESPACE_END
