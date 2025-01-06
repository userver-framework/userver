#pragma once

#include <userver/utest/utest.hpp>

#include <unordered_map>

#include <fmt/format.h>

#include <userver/server/http/http_request.hpp>

#include <server/http/http2_session.hpp>
#include <server/http/http_request_parser.hpp>
#include <server/net/connection_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace server {

inline std::shared_ptr<request::RequestParser> CreateTestParser(
    server::http::HttpRequestParser::OnNewRequestCb&& cb,
    USERVER_NAMESPACE::http::HttpVersion http_version = USERVER_NAMESPACE::http::HttpVersion::k11
) {
    static const server::http::HandlerInfoIndex kTestHandlerInfoIndex;
    static const server::request::HttpRequestConfig kTestRequestConfig{
        /*.max_url_size = */ 8192,
        /*.max_request_size = */ 1024 * 1024,
        /*.max_headers_size = */ 65536,
        /*.parse_args_from_body = */ false,
        /*.testing_mode = */ true,  // non default value
        /*.decompress_request = */ false,
        /* set_tracing_headers = */ true,
        /* deadline_propagation_enabled = */ true,
        /* deadline_expired_status_code = */ http::HttpStatus{498}};
    static server::net::ParserStats test_stats;
    static server::request::ResponseDataAccounter test_accounter;
    static const server::net::Http2SessionConfig http2_config;
    if (http_version == USERVER_NAMESPACE::http::HttpVersion::k2) {
        return std::make_shared<server::http::Http2Session>(
            kTestHandlerInfoIndex,
            kTestRequestConfig,
            http2_config,
            std::move(cb),
            test_stats,
            test_accounter,
            engine::io::Sockaddr{}
        );
    } else {
        return std::make_shared<server::http::HttpRequestParser>(
            kTestHandlerInfoIndex, kTestRequestConfig, std::move(cb), test_stats, test_accounter, engine::io::Sockaddr{}
        );
    }
}

}  // namespace server

USERVER_NAMESPACE_END
