#include <userver/dynamic_config/test_helpers.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/utest/http_client.hpp>
#include <userver/utest/utest.hpp>

#include <userver/chaotic/openapi/middlewares/follow_redirects_middleware.hpp>
#include <userver/chaotic/openapi/middlewares/logging_middleware.hpp>
#include <userver/chaotic/openapi/middlewares/manager.hpp>
#include <userver/chaotic/openapi/middlewares/proxy_middleware.hpp>
#include <userver/chaotic/openapi/middlewares/ssl_middleware.hpp>
#include <userver/chaotic/openapi/middlewares/timeout_retry_middleware.hpp>

#include <userver/clients/http/client.hpp>
#include <userver/clients/http/request.hpp>
#include <userver/clients/http/response.hpp>

USERVER_NAMESPACE_BEGIN

UTEST(Middlewares, TimeoutRetrySchema) {
    const std::string schema = chaotic::openapi::TimeoutRetryMiddleware::GetStaticConfigSchemaStr();

    EXPECT_FALSE(schema.empty());
    EXPECT_NE(schema.find("timeout_ms"), std::string::npos);
    EXPECT_NE(schema.find("retries"), std::string::npos);
}

UTEST(Middlewares, FollowRedirectsSchema) {
    const std::string schema = chaotic::openapi::FollowRedirectsMiddleware::GetStaticConfigSchemaStr();

    EXPECT_FALSE(schema.empty());
    EXPECT_NE(schema.find("enabled"), std::string::npos);
}

UTEST(Middlewares, ProxySchema) {
    const std::string schema = chaotic::openapi::ProxyMiddleware::GetStaticConfigSchemaStr();

    EXPECT_FALSE(schema.empty());
    EXPECT_NE(schema.find("url"), std::string::npos);
}

UTEST(Middlewares, SslSchema) {
    const std::string schema = chaotic::openapi::SslMiddleware::GetStaticConfigSchemaStr();

    EXPECT_FALSE(schema.empty());
    EXPECT_NE(schema.find("certificate"), std::string::npos);
}

UTEST(Middlewares, LoggingSchema) {
    const std::string schema = chaotic::openapi::LoggingMiddleware::GetStaticConfigSchemaStr();

    EXPECT_FALSE(schema.empty());
    EXPECT_NE(schema.find("request_level"), std::string::npos);
    EXPECT_NE(schema.find("response_level"), std::string::npos);
    EXPECT_NE(schema.find("body_limit"), std::string::npos);
}

UTEST(Middlewares, Manager) {
    chaotic::openapi::MiddlewareManager manager;

    auto timeout_retry = std::make_shared<chaotic::openapi::TimeoutRetryMiddleware>(std::chrono::milliseconds(100), 3);
    auto follow_redirects = std::make_shared<chaotic::openapi::FollowRedirectsMiddleware>(true);
    auto logging =
        std::make_shared<chaotic::openapi::LoggingMiddleware>(logging::Level::kDebug, logging::Level::kDebug, 1024);

    manager.RegisterMiddleware(timeout_retry);
    manager.RegisterMiddleware(follow_redirects);
    manager.RegisterMiddleware(logging);
}

UTEST(Middlewares, TimeoutRetryFactory) {
    chaotic::openapi::TimeoutRetryMiddlewareFactory factory;

    EXPECT_FALSE(factory.GetStaticConfigSchemaStr().empty());
}

UTEST(Middlewares, FollowRedirectsFactory) {
    chaotic::openapi::FollowRedirectsMiddlewareFactory factory;

    EXPECT_FALSE(factory.GetStaticConfigSchemaStr().empty());
}

UTEST(Middlewares, ProxyFactory) {
    chaotic::openapi::ProxyMiddlewareFactory factory;

    EXPECT_FALSE(factory.GetStaticConfigSchemaStr().empty());
}

UTEST(Middlewares, SslFactory) {
    chaotic::openapi::SslMiddlewareFactory factory;

    EXPECT_FALSE(factory.GetStaticConfigSchemaStr().empty());
}

UTEST(Middlewares, LoggingFactory) {
    chaotic::openapi::LoggingMiddlewareFactory factory;

    EXPECT_FALSE(factory.GetStaticConfigSchemaStr().empty());
}

USERVER_NAMESPACE_END
