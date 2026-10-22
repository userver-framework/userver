#include <userver/utest/utest.hpp>

#include <string>
#include <string_view>
#include <type_traits>

#include <userver/formats/json/serialize.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/utest/http_client.hpp>
#include <userver/utest/http_server_mock.hpp>

#include <test/tagged/admin/handlers/writecredential/handler.hpp>
#include <test/tagged/api/client/client_impl.hpp>
#include <test/tagged/api/handlers/usecredential/handler.hpp>
#include <test/tagged/api/handlers/writecredential/handler.hpp>
#include <test/tagged/dto/credentials.hpp>
#include <test/tagged/errors/errors.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

namespace client = ::test::tagged::api::client::writecredential;
namespace api_handler = ::test::tagged::api::handlers::writecredential;
namespace admin_handler = ::test::tagged::admin::handlers::writecredential;
namespace use_client = ::test::tagged::api::client::usecredential;
namespace use_handler = ::test::tagged::api::handlers::usecredential;

// A shared document is generated once and every surface refers to the same type
static_assert(std::is_same_v<client::Body, ::test::tagged::dto::Credential>);
static_assert(std::is_same_v<decltype(api_handler::Request::body), ::test::tagged::dto::Credential>);
static_assert(std::is_same_v<decltype(admin_handler::Request::body), ::test::tagged::dto::Credential>);
static_assert(std::is_same_v<decltype(api_handler::Response400::body), ::test::tagged::errors::Error>);
static_assert(std::is_same_v<
              decltype(::test::tagged::dto::Credential::last_error)::value_type,
              ::test::tagged::errors::Error>);

// An inline type of a document is generated once, in the document's namespace, and both surfaces refer to it
using WrittenBody = ::test::tagged::api::api_credentials::put::Response200BodyApplicationJson;
static_assert(std::is_same_v<decltype(client::Response200::body), WrittenBody>);
static_assert(std::is_same_v<decltype(api_handler::Response200::body), WrittenBody>);

// A media type without a schema carries any JSON value
static_assert(std::is_same_v<decltype(use_client::Request::body), formats::json::Value>);
static_assert(std::is_same_v<decltype(use_client::Response200::body), formats::json::Value>);
static_assert(std::is_same_v<decltype(use_handler::Request::body), formats::json::Value>);
static_assert(std::is_same_v<decltype(use_handler::Response200::body), formats::json::Value>);

// The client and the handlers of one document are different surfaces in their own namespaces
static_assert(!std::is_same_v<client::Request, api_handler::Request>);
static_assert(!std::is_same_v<api_handler::Request, admin_handler::Request>);

}  // namespace

UTEST(Tagged, HandlerNamesFollowNamespace) {
    EXPECT_EQ(api_handler::impl::kHandlerName, "test-tagged-api-handlers-write-credential");
    EXPECT_EQ(admin_handler::impl::kHandlerName, "test-tagged-admin-handlers-write-credential");
}

UTEST(Tagged, ClientSendsSharedType) {
    std::string received_path;
    std::string received_body;
    const utest::HttpServerMock http_server{[&](const utest::HttpServerMock::HttpRequest& request) {
        received_path = request.path;
        received_body = request.body;
        utest::HttpServerMock::HttpResponse response;
        response.response_status = 200;
        response.headers[http::headers::kContentType] = "application/json";
        response.body = R"({"written":true})";
        return response;
    }};

    chaotic::openapi::client::Config config;
    config.base_url = http_server.GetBaseUrl();
    auto http_client = utest::CreateHttpClient();
    ::test::tagged::api::client::ClientImpl api(config, *http_client);

    const auto response = api.writeCredential({::test::tagged::dto::Credential{"key", {}}});

    EXPECT_EQ(received_path, "/api/credentials");
    EXPECT_EQ(formats::json::FromString(received_body), formats::json::FromString(R"({"name":"key"})"));
    EXPECT_EQ(response.body.written, true);
}

UTEST(Tagged, ClientPassesBodyWithoutSchema) {
    std::string received_path;
    std::string received_body;
    const utest::HttpServerMock http_server{[&](const utest::HttpServerMock::HttpRequest& request) {
        received_path = request.path;
        received_body = request.body;
        utest::HttpServerMock::HttpResponse response;
        response.response_status = 200;
        response.headers[http::headers::kContentType] = "application/json";
        response.body = R"({"token":"secret","ttl":[1,2]})";
        return response;
    }};

    chaotic::openapi::client::Config config;
    config.base_url = http_server.GetBaseUrl();
    auto http_client = utest::CreateHttpClient();
    ::test::tagged::api::client::ClientImpl api(config, *http_client);

    const auto response = api.useCredential({"key", formats::json::FromString(R"({"purpose":"test"})")});

    EXPECT_EQ(received_path, "/api/credentials/key/use");
    EXPECT_EQ(formats::json::FromString(received_body), formats::json::FromString(R"({"purpose":"test"})"));
    EXPECT_EQ(response.body, formats::json::FromString(R"({"token":"secret","ttl":[1,2]})"));
}

USERVER_NAMESPACE_END
