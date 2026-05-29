#include <userver/utest/utest.hpp>

#include <userver/chaotic/io/userver/decimal64/decimal.hpp>
#include <userver/chaotic/openapi/parameters_read.hpp>
#include <userver/server/http/http_request_builder.hpp>
#include <userver/utils/algo.hpp>

USERVER_NAMESPACE_BEGIN

namespace co = chaotic::openapi;

static constexpr co::Name kName{"foo"};

UTEST(OpenapiParametersRead, SourceHttpRequest) {
    auto request =
        server::http::HttpRequestBuilder()
            .AddRequestArg("foo", "bar")
            .AddRequestArg("foo", "baz")
            .AddHeader("foo", "header")
            .AddHeader("Cookie", "foo=cookie")
            .SetPathArgs({{"foo", "path"}})
            .Build();
    const auto& source = *request;

    auto query_multi = co::ReadParameter<co::ArrayParameter<co::In::kQueryExplode, kName, ',', std::string>>(source);
    auto query = co::ReadParameter<co::TrivialParameter<co::In::kQuery, kName, std::string>>(source);
    auto path = co::ReadParameter<co::TrivialParameter<co::In::kPath, kName, std::string>>(source);
    auto header = co::ReadParameter<co::TrivialParameter<co::In::kHeader, kName, std::string>>(source);
    auto cookie = co::ReadParameter<co::TrivialParameter<co::In::kCookie, kName, std::string>>(source);

    EXPECT_EQ(query_multi, (std::vector<std::string>{"bar", "baz"}));
    EXPECT_EQ(query, "bar");
    EXPECT_EQ(path, "path");
    EXPECT_EQ(header, "header");
    EXPECT_EQ(cookie, "cookie");
}

UTEST(OpenapiParametersRead, TypeBoolean) {
    auto request = server::http::HttpRequestBuilder().AddRequestArg("foo", "true").Build();

    const bool foo{co::ReadParameter<co::TrivialParameter<co::In::kQuery, kName, bool>>(*request)};
    EXPECT_EQ(foo, true);
}

UTEST(OpenapiParametersRead, TypeDouble) {
    auto request = server::http::HttpRequestBuilder().AddRequestArg("foo", "12.2").Build();

    const double foo{co::ReadParameter<co::TrivialParameter<co::In::kQuery, kName, double>>(*request)};
    EXPECT_EQ(foo, 12.2);
}

UTEST(OpenapiParametersRead, UserType) {
    auto request = server::http::HttpRequestBuilder().AddRequestArg("foo", "12.2").Build();

    using Decimal = decimal64::Decimal<10>;
    const Decimal foo{co::ReadParameter<co::TrivialParameter<co::In::kQuery, kName, std::string, Decimal>>(*request)};
    EXPECT_EQ(foo, Decimal{"12.2"});
}

UTEST(OpenapiParametersRead, TypeInt) {
    auto request = server::http::HttpRequestBuilder().AddRequestArg("foo", "12").Build();

    const int foo{co::ReadParameter<co::TrivialParameter<co::In::kQuery, kName, int>>(*request)};
    EXPECT_EQ(foo, 12);
}

UTEST(OpenapiParametersRead, TypeArrayOfString) {
    auto request = server::http::HttpRequestBuilder().AddRequestArg("foo", "bar,baz").Build();

    const std::vector<std::string> foo{
        co::ReadParameter<co::ArrayParameter<co::In::kQuery, kName, ',', std::string>>(*request)
    };
    EXPECT_EQ(foo, (std::vector<std::string>{"bar", "baz"}));
}

UTEST(OpenapiParametersRead, TypeArrayOfUser) {
    auto request = server::http::HttpRequestBuilder().AddRequestArg("foo", "1.2,3.4").Build();

    using Decimal = decimal64::Decimal<10>;
    const std::vector<Decimal> foo{
        co::ReadParameter<co::ArrayParameter<co::In::kQuery, kName, ',', std::string, Decimal>>(*request)
    };
    EXPECT_EQ(foo, (std::vector<Decimal>{Decimal{"1.2"}, Decimal{"3.4"}}));
}

UTEST(OpenapiParametersRead, TypeStringExplode) {
    auto request =
        server::http::HttpRequestBuilder()
            .AddRequestArg("foo", "1")
            .AddRequestArg("foo", "2")
            .AddRequestArg("foo", "3")
            .Build();

    const std::vector<int> foo{co::ReadParameter<co::ArrayParameter<co::In::kQueryExplode, kName, ',', int>>(*request)};
    EXPECT_EQ(foo, (std::vector{1, 2, 3}));
}

// --- IsParameterPresent ---

UTEST(OpenapiParametersRead, IsParameterPresentQueryPresent) {
    auto request = server::http::HttpRequestBuilder().AddRequestArg("foo", "val").Build();
    EXPECT_TRUE(co::IsParameterPresent<co::In::kQuery>("foo", *request));
    EXPECT_FALSE(co::IsParameterPresent<co::In::kQuery>("bar", *request));
}

UTEST(OpenapiParametersRead, IsParameterPresentQueryExplodePresent) {
    auto request = server::http::HttpRequestBuilder().AddRequestArg("foo", "v").Build();
    EXPECT_TRUE(co::IsParameterPresent<co::In::kQueryExplode>("foo", *request));
    EXPECT_FALSE(co::IsParameterPresent<co::In::kQueryExplode>("bar", *request));
}

UTEST(OpenapiParametersRead, IsParameterPresentHeaderPresent) {
    auto request = server::http::HttpRequestBuilder().AddHeader("foo", "val").Build();
    EXPECT_TRUE(co::IsParameterPresent<co::In::kHeader>("foo", *request));
    EXPECT_FALSE(co::IsParameterPresent<co::In::kHeader>("bar", *request));
}

UTEST(OpenapiParametersRead, IsParameterPresentCookiePresent) {
    auto request = server::http::HttpRequestBuilder().AddHeader("Cookie", "foo=val").Build();
    EXPECT_TRUE(co::IsParameterPresent<co::In::kCookie>("foo", *request));
    EXPECT_FALSE(co::IsParameterPresent<co::In::kCookie>("bar", *request));
}

UTEST(OpenapiParametersRead, IsParameterPresentPathAlwaysTrue) {
    auto request = server::http::HttpRequestBuilder().Build();
    EXPECT_TRUE(co::IsParameterPresent<co::In::kPath>("anything", *request));
}

// --- ReadParameter: missing required → ClientError ---

UTEST(OpenapiParametersRead, RequiredQueryMissingThrows) {
    auto request = server::http::HttpRequestBuilder().Build();
    UEXPECT_THROW(
        (co::ReadParameter<co::TrivialParameter<co::In::kQuery, kName, std::string>>(*request)),
        server::handlers::ClientError
    );
}

UTEST(OpenapiParametersRead, RequiredHeaderMissingThrows) {
    auto request = server::http::HttpRequestBuilder().Build();
    UEXPECT_THROW(
        (co::ReadParameter<co::TrivialParameter<co::In::kHeader, kName, std::string>>(*request)),
        server::handlers::ClientError
    );
}

UTEST(OpenapiParametersRead, RequiredCookieMissingThrows) {
    auto request = server::http::HttpRequestBuilder().Build();
    UEXPECT_THROW(
        (co::ReadParameter<co::TrivialParameter<co::In::kCookie, kName, std::string>>(*request)),
        server::handlers::ClientError
    );
}

// --- ReadParameterOptional: absent → nullopt, present → value ---

UTEST(OpenapiParametersRead, OptionalQueryAbsentReturnsNullopt) {
    auto request = server::http::HttpRequestBuilder().Build();
    auto val = co::ReadParameterOptional<co::TrivialParameter<co::In::kQuery, kName, std::string>>(*request);
    EXPECT_FALSE(val.has_value());
}

UTEST(OpenapiParametersRead, OptionalQueryPresentReturnsValue) {
    auto request = server::http::HttpRequestBuilder().AddRequestArg("foo", "bar").Build();
    auto val = co::ReadParameterOptional<co::TrivialParameter<co::In::kQuery, kName, std::string>>(*request);
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(*val, "bar");
}

UTEST(OpenapiParametersRead, OptionalQueryExplicitlyEmptyReturnsEmpty) {
    // An explicitly provided empty query arg is distinct from absent.
    auto request = server::http::HttpRequestBuilder().AddRequestArg("foo", "").Build();
    auto val = co::ReadParameterOptional<co::TrivialParameter<co::In::kQuery, kName, std::string>>(*request);
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(*val, "");
}

UTEST(OpenapiParametersRead, OptionalHeaderAbsentReturnsNullopt) {
    auto request = server::http::HttpRequestBuilder().Build();
    auto val = co::ReadParameterOptional<co::TrivialParameter<co::In::kHeader, kName, std::string>>(*request);
    EXPECT_FALSE(val.has_value());
}

UTEST(OpenapiParametersRead, OptionalHeaderPresentReturnsValue) {
    auto request = server::http::HttpRequestBuilder().AddHeader("foo", "hval").Build();
    auto val = co::ReadParameterOptional<co::TrivialParameter<co::In::kHeader, kName, std::string>>(*request);
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(*val, "hval");
}

UTEST(OpenapiParametersRead, OptionalCookieAbsentReturnsNullopt) {
    auto request = server::http::HttpRequestBuilder().Build();
    auto val = co::ReadParameterOptional<co::TrivialParameter<co::In::kCookie, kName, std::string>>(*request);
    EXPECT_FALSE(val.has_value());
}

UTEST(OpenapiParametersRead, OptionalExplodeAbsentReturnsNullopt) {
    auto request = server::http::HttpRequestBuilder().Build();
    auto val = co::ReadParameterOptional<co::ArrayParameter<co::In::kQueryExplode, kName, ',', std::string>>(*request);
    EXPECT_FALSE(val.has_value());
}

UTEST(OpenapiParametersRead, OptionalExplodePresentReturnsVector) {
    auto request = server::http::HttpRequestBuilder().AddRequestArg("foo", "a").AddRequestArg("foo", "b").Build();
    auto val = co::ReadParameterOptional<co::ArrayParameter<co::In::kQueryExplode, kName, ',', std::string>>(*request);
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(*val, (std::vector<std::string>{"a", "b"}));
}

USERVER_NAMESPACE_END
