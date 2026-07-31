#include <userver/utest/utest.hpp>

#include <userver/server/http/form_data_arg.hpp>
#include <userver/server/http/http_request_builder.hpp>
#include <userver/utest/assert_macros.hpp>
#include <userver/utils/impl/transparent_hash.hpp>

#include <handlers/simple/formpost/handler.hpp>
#include <handlers/simple/formpost/requests.hpp>
#include <handlers/simple/formpost/responses.hpp>
#include <handlers/simple/headersget/handler.hpp>
#include <handlers/simple/headersget/requests.hpp>
#include <handlers/simple/headersget/responses.hpp>
#include <handlers/simple/multipartpost/handler.hpp>
#include <handlers/simple/multipartpost/requests.hpp>
#include <handlers/simple/multipartpost/responses.hpp>
#include <handlers/simple/octetget/handler.hpp>
#include <handlers/simple/octetget/requests.hpp>
#include <handlers/simple/octetget/responses.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

namespace headersget = ::handlers::simple::headersget;
namespace octetget = ::handlers::simple::octetget;
namespace formpost = ::handlers::simple::formpost;
namespace multipartpost = ::handlers::simple::multipartpost;

// --- octet-stream request body ---

UTEST(HandlerBodyOctetStream, ParsesBody) {
    auto request = server::http::HttpRequestBuilder{}.SetBody("binary\x00data").Build();

    auto parsed = octetget::ParseRequest(*request, chaotic::openapi::To<octetget::Request>{});
    EXPECT_EQ(parsed.body.data, "binary\x00data");
}

UTEST(HandlerBodyOctetStream, ParsesEmptyBody) {
    auto request = server::http::HttpRequestBuilder{}.SetBody("").Build();

    auto parsed = octetget::ParseRequest(*request, chaotic::openapi::To<octetget::Request>{});
    EXPECT_EQ(parsed.body.data, "");
}

// --- octet-stream response body ---

UTEST(HandlerBodyOctetStream, SerializeResponseSetsContentType) {
    auto builder = server::http::HttpRequestBuilder{};
    auto request = builder.Build();

    const std::string body = octetget::SerializeResponse(octetget::Response200{"hello"}, *request);

    EXPECT_EQ(body, "hello");
    EXPECT_EQ(builder.GetHttpResponse().GetHeader("Content-Type"), "application/octet-stream");
}

// --- form-urlencoded request body ---

UTEST(HandlerBodyFormUrlencoded, ParsesRequiredAndOptionalFields) {
    auto request =
        server::http::HttpRequestBuilder{}.AddRequestArg("name", "alice").AddRequestArg("count", "42").Build();

    auto parsed = formpost::ParseRequest(*request, chaotic::openapi::To<formpost::Request>{});
    EXPECT_EQ(parsed.body.name, "alice");
    ASSERT_TRUE(parsed.body.count.has_value());
    EXPECT_EQ(*parsed.body.count, 42);
}

UTEST(HandlerBodyFormUrlencoded, ParsesWithoutOptionalField) {
    auto request = server::http::HttpRequestBuilder{}.AddRequestArg("name", "bob").Build();

    auto parsed = formpost::ParseRequest(*request, chaotic::openapi::To<formpost::Request>{});
    EXPECT_EQ(parsed.body.name, "bob");
    EXPECT_FALSE(parsed.body.count.has_value());
}

UTEST(HandlerBodyFormUrlencoded, ThrowsOnMissingRequiredField) {
    auto request = server::http::HttpRequestBuilder{}.Build();

    UEXPECT_THROW(
        formpost::ParseRequest(*request, chaotic::openapi::To<formpost::Request>{}),
        server::handlers::ClientError
    );
}

// --- multipart/form-data request body ---

UTEST(HandlerBodyMultipartFormData, ParsesRequiredAndOptionalFields) {
    utils::impl::TransparentMap<std::string, std::vector<server::http::FormDataArg>, utils::StrCaseHash> form_data;
    server::http::FormDataArg name_arg{};
    name_arg.value = "charlie";
    form_data["name"] = {name_arg};
    server::http::FormDataArg count_arg{};
    count_arg.value = "7";
    form_data["count"] = {count_arg};

    auto request = server::http::HttpRequestBuilder{}.SetFormDataArgs(std::move(form_data)).Build();

    auto parsed = multipartpost::ParseRequest(*request, chaotic::openapi::To<multipartpost::Request>{});
    EXPECT_EQ(parsed.body.name, "charlie");
    ASSERT_TRUE(parsed.body.count.has_value());
    EXPECT_EQ(*parsed.body.count, 7);
}

UTEST(HandlerBodyMultipartFormData, ParsesWithoutOptionalField) {
    utils::impl::TransparentMap<std::string, std::vector<server::http::FormDataArg>, utils::StrCaseHash> form_data;
    server::http::FormDataArg name_arg{};
    name_arg.value = "dave";
    form_data["name"] = {name_arg};

    auto request = server::http::HttpRequestBuilder{}.SetFormDataArgs(std::move(form_data)).Build();

    auto parsed = multipartpost::ParseRequest(*request, chaotic::openapi::To<multipartpost::Request>{});
    EXPECT_EQ(parsed.body.name, "dave");
    EXPECT_FALSE(parsed.body.count.has_value());
}

UTEST(HandlerBodyMultipartFormData, ThrowsOnMissingRequiredField) {
    auto request = server::http::HttpRequestBuilder{}.Build();

    UEXPECT_THROW(
        multipartpost::ParseRequest(*request, chaotic::openapi::To<multipartpost::Request>{}),
        server::handlers::ClientError
    );
}

// --- response headers ---

UTEST(HandlerResponseHeaders, RequiredHeaderIsSet) {
    auto builder = server::http::HttpRequestBuilder{};
    auto request = builder.Build();

    headersget::Response200 r{};
    r.body = "hello";
    r.X_String = "value";
    headersget::SerializeResponse(r, *request);

    EXPECT_EQ(builder.GetHttpResponse().GetHeader("X-String"), "value");
}

UTEST(HandlerResponseHeaders, OptionalHeaderPresentWhenSet) {
    auto builder = server::http::HttpRequestBuilder{};
    auto request = builder.Build();

    headersget::Response200 r{};
    r.body = "hello";
    r.X_String = "v";
    r.X_Count = 42;
    headersget::SerializeResponse(r, *request);

    EXPECT_EQ(builder.GetHttpResponse().GetHeader("X-Count"), "42");
}

UTEST(HandlerResponseHeaders, OptionalHeaderAbsentWhenNotSet) {
    auto builder = server::http::HttpRequestBuilder{};
    auto request = builder.Build();

    headersget::Response200 r{};
    r.body = "hello";
    r.X_String = "v";
    // X_Count not set
    headersget::SerializeResponse(r, *request);

    EXPECT_EQ(builder.GetHttpResponse().GetHeader("X-Count"), "");
}

}  // namespace

USERVER_NAMESPACE_END
