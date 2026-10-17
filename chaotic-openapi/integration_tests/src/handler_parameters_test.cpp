#include <userver/utest/utest.hpp>

#include <chrono>
#include <cstdint>
#include <string>
#include <string_view>

#include <vector>

#include <boost/uuid/string_generator.hpp>
#include <fmt/format.h>

#include <userver/chaotic/io/test/account_id.hpp>
#include <userver/chaotic/io/test/count.hpp>
#include <userver/chaotic/io/test/label.hpp>
#include <userver/chaotic/io/test/station_id.hpp>
#include <userver/server/http/http_request_builder.hpp>

#include <handlers/simple/sharedget/handler.hpp>
#include <handlers/simple/sharedget/requests.hpp>
#include <handlers/simple/sharedput/handler.hpp>
#include <handlers/simple/sharedput/requests.hpp>
#include <handlers/simple/strongget/handler.hpp>
#include <handlers/simple/strongget/requests.hpp>
#include <handlers/simple/strongget/responses.hpp>
#include <handlers/simple/typedget/handler.hpp>
#include <handlers/simple/typedget/requests.hpp>
#include <handlers/simple/typedget/responses.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

namespace sharedget = ::handlers::simple::sharedget;
namespace sharedput = ::handlers::simple::sharedput;
namespace strongget = ::handlers::simple::strongget;
namespace typedget = ::handlers::simple::typedget;

constexpr std::string_view kId = "6ba7b810-9dad-11d1-80b4-00c04fd430c8";
constexpr std::string_view kAccount = "6ba7b811-9dad-11d1-80b4-00c04fd430c8";
constexpr std::string_view kStation = "6ba7b812-9dad-11d1-80b4-00c04fd430c8";
constexpr std::string_view kTraceId = "6ba7b813-9dad-11d1-80b4-00c04fd430c8";

boost::uuids::uuid ToUuid(std::string_view value) { return boost::uuids::string_generator{}(value.begin(), value.end()); }

UTEST(HandlerParameters, UserCppTypesAreParsed) {
    auto request = server::http::HttpRequestBuilder{}
                       .SetPathArgs({{"id", std::string{kId}}})
                       .AddRequestArg("account", std::string{kAccount})
                       .AddRequestArg("station", std::string{kStation})
                       .AddHeader("X-Trace-Id", std::string{kTraceId})
                       .Build();

    const auto parsed = typedget::ParseRequest(*request, chaotic::openapi::To<typedget::Request>{});

    EXPECT_EQ(parsed.id, ToUuid(kId));
    EXPECT_EQ(parsed.account, ToUuid(kAccount));
    EXPECT_EQ(parsed.station, ToUuid(kStation));
    EXPECT_EQ(parsed.X_Trace_Id, ToUuid(kTraceId));
}

UTEST(HandlerParameters, OptionalUserCppTypeIsAbsent) {
    auto request = server::http::HttpRequestBuilder{}
                       .SetPathArgs({{"id", std::string{kId}}})
                       .AddRequestArg("account", std::string{kAccount})
                       .AddHeader("X-Trace-Id", std::string{kTraceId})
                       .Build();

    const auto parsed = typedget::ParseRequest(*request, chaotic::openapi::To<typedget::Request>{});

    EXPECT_FALSE(parsed.station.has_value());
}

UTEST(HandlerParameters, UserCppTypeHeaderIsSerialized) {
    auto builder = server::http::HttpRequestBuilder{};
    auto request = builder.Build();

    typedget::Response200 response;
    response.X_Request_Id = ToUuid(kId);
    typedget::SerializeResponse(response, *request);

    EXPECT_EQ(builder.GetHttpResponse().GetHeader("X-Request-Id"), kId);
}

UTEST(HandlerParameters, StrongTypedefsAndChronoAreParsed) {
    auto request = server::http::HttpRequestBuilder{}
                       .SetPathArgs({{"account", std::string{kAccount}}})
                       .AddRequestArg("station", std::string{kStation})
                       .AddRequestArg("accounts", fmt::format("{},{}", kId, kAccount))
                       .AddRequestArg("count", "42")
                       .AddRequestArg("label", "hello")
                       .AddRequestArg("timeout", "1500")
                       .AddHeader("X-Account-Id", std::string{kTraceId})
                       .Build();

    const auto parsed = strongget::ParseRequest(*request, chaotic::openapi::To<strongget::Request>{});

    EXPECT_EQ(parsed.account, ::test::AccountId{ToUuid(kAccount)});
    EXPECT_EQ(parsed.station.GetUnderlying(), ToUuid(kStation));
    EXPECT_EQ(parsed.accounts, (std::vector{::test::AccountId{ToUuid(kId)}, ::test::AccountId{ToUuid(kAccount)}}));
    EXPECT_EQ(parsed.count.GetUnderlying(), 42);
    EXPECT_EQ(parsed.label.GetUnderlying(), "hello");
    EXPECT_EQ(parsed.timeout, std::chrono::milliseconds{1500});
    EXPECT_EQ(parsed.X_Account_Id, ::test::AccountId{ToUuid(kTraceId)});
}

UTEST(HandlerParameters, StrongTypedefAndChronoHeadersAreSerialized) {
    auto builder = server::http::HttpRequestBuilder{};
    auto request = builder.Build();

    strongget::Response200 response;
    response.X_Account_Id = ::test::AccountId{ToUuid(kAccount)};
    response.X_Retry_After = std::chrono::seconds{30};
    strongget::SerializeResponse(response, *request);

    EXPECT_EQ(builder.GetHttpResponse().GetHeader("X-Account-Id"), kAccount);
    EXPECT_EQ(builder.GetHttpResponse().GetHeader("X-Retry-After"), "30");
}

UTEST(HandlerParameters, PathItemParametersAreSharedByOperations) {
    auto get_request = server::http::HttpRequestBuilder{}
                           .SetPathArgs({{"spot", std::string{kId}}})
                           .AddRequestArg("unit", "metric")
                           .Build();
    const auto get_parsed = sharedget::ParseRequest(*get_request, chaotic::openapi::To<sharedget::Request>{});
    EXPECT_EQ(get_parsed.spot, ToUuid(kId));
    EXPECT_EQ(get_parsed.unit, "metric");

    auto put_request = server::http::HttpRequestBuilder{}
                           .SetPathArgs({{"spot", std::string{kId}}})
                           .AddRequestArg("unit", "imperial")
                           .AddRequestArg("extra", "42")
                           .Build();
    const auto put_parsed = sharedput::ParseRequest(*put_request, chaotic::openapi::To<sharedput::Request>{});
    EXPECT_EQ(put_parsed.spot, ToUuid(kId));
    EXPECT_EQ(put_parsed.unit, "imperial");
    EXPECT_EQ(put_parsed.extra, 42);
}

}  // namespace

USERVER_NAMESPACE_END
