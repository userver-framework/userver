#include <userver/utest/utest.hpp>

#include <string>
#include <string_view>

#include <boost/uuid/string_generator.hpp>

#include <userver/server/http/http_request_builder.hpp>

#include <handlers/simple/typedget/handler.hpp>
#include <handlers/simple/typedget/requests.hpp>
#include <handlers/simple/typedget/responses.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

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

}  // namespace

USERVER_NAMESPACE_END
