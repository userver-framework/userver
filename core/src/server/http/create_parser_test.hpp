#pragma once

#include <userver/utest/utest.hpp>

#include <unordered_map>

#include <fmt/format.h>

#include <userver/server/http/http_request.hpp>

#include <server/http/http_request_parser.hpp>

USERVER_NAMESPACE_BEGIN

namespace server {

inline server::http::HttpRequestParser CreateTestParser(
    server::http::HttpRequestParser::OnNewRequestCb&& cb) {
  static const server::http::HandlerInfoIndex kTestHandlerInfoIndex;
  static constexpr server::request::HttpRequestConfig kTestRequestConfig{
      /*.max_url_size = */ 8192,
      /*.max_request_size = */ 1024 * 1024,
      /*.max_headers_size = */ 65536,
      /*.parse_args_from_body = */ false,
      /*.testing_mode = */ true,  // non default value
      /*.decompress_request = */ false,
  };
  static server::net::ParserStats test_stats;
  static server::request::ResponseDataAccounter test_accounter;
  return server::http::HttpRequestParser(kTestHandlerInfoIndex,
                                         kTestRequestConfig, std::move(cb),
                                         test_stats, test_accounter);
}

}  // namespace server

USERVER_NAMESPACE_END
