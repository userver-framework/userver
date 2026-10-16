#include <userver/utest/utest.hpp>

#include <chrono>
#include <string>
#include <string_view>

#include <boost/uuid/string_generator.hpp>
#include <fmt/format.h>

#include <clients/parameters/requests.hpp>
#include <clients/parameters/responses.hpp>
#include <userver/chaotic/io/test/account_id.hpp>
#include <userver/chaotic/io/test/count.hpp>
#include <userver/chaotic/io/test/label.hpp>
#include <userver/chaotic/io/test/station_id.hpp>
#include <userver/clients/http/client.hpp>
#include <userver/http/predefined_header.hpp>
#include <userver/utest/http_client.hpp>
#include <userver/utest/http_server_mock.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

namespace client = ::clients::parameters::test1::post;
namespace typed_client = ::clients::parameters::test1_typed_id::get;
namespace strong_client = ::clients::parameters::test1_strong_account::get;

constexpr std::string_view kId = "6ba7b810-9dad-11d1-80b4-00c04fd430c8";
constexpr std::string_view kAccount = "6ba7b811-9dad-11d1-80b4-00c04fd430c8";
constexpr std::string_view kStation = "6ba7b812-9dad-11d1-80b4-00c04fd430c8";
constexpr std::string_view kTraceId = "6ba7b813-9dad-11d1-80b4-00c04fd430c8";
constexpr http::headers::PredefinedHeader kTraceIdHeader{"X-Trace-Id"};
constexpr http::headers::PredefinedHeader kAccountIdHeader{"X-Account-Id"};

boost::uuids::uuid ToUuid(std::string_view value) { return boost::uuids::string_generator{}(value.begin(), value.end()); }

UTEST(Parameters, CppName) {
    client::Request request;

    // compilation test
    request.myclass = "123";
}

UTEST(Parameters, Explode) {
    client::Request request;
    auto http_client_ptr = utest::CreateHttpClient();
    auto http_request = http_client_ptr->CreateRequest();

    request.multiple = {"1", "2", "3"};
    client::SerializeRequest(request, "http://base", http_request);

    EXPECT_EQ(http_request.GetUrl(), "http://base/test1?multiple=1&multiple=2&multiple=3");
}

UTEST(Parameters, UserCppTypesAreSerialized) {
    typed_client::Request request;
    auto http_client_ptr = utest::CreateHttpClient();
    auto http_request = http_client_ptr->CreateRequest();

    request.id = ToUuid(kId);
    request.account = ToUuid(kAccount);
    request.station = ToUuid(kStation);
    typed_client::SerializeRequest(request, "http://base", http_request);

    EXPECT_EQ(
        http_request.GetUrl(),
        fmt::format("http://base/test1/typed/{}?account={}&station={}", kId, kAccount, kStation)
    );
}

UTEST(Parameters, UserCppTypeRequestHeaderIsSerialized) {
    std::string received_trace_id;
    const utest::HttpServerMock http_server([&received_trace_id](const utest::HttpServerMock::HttpRequest& request) {
        const auto it = request.headers.find(kTraceIdHeader);
        if (it != request.headers.end()) {
            received_trace_id = it->second;
        }
        utest::HttpServerMock::HttpResponse r;
        r.response_status = 200;
        return r;
    });
    auto http_client_ptr = utest::CreateHttpClient();
    auto http_request = http_client_ptr->CreateRequest();

    typed_client::Request request;
    request.X_Trace_Id = ToUuid(kTraceId);
    typed_client::SerializeRequest(request, http_server.GetBaseUrl(), http_request);
    EXPECT_TRUE(http_request.perform()->IsOk());

    EXPECT_EQ(received_trace_id, kTraceId);
}

UTEST(Parameters, UserCppTypeHeaderIsParsed) {
    const utest::HttpServerMock http_server([](const utest::HttpServerMock::HttpRequest&) {
        utest::HttpServerMock::HttpResponse r;
        r.response_status = 200;
        r.headers[std::string{"X-Request-Id"}] = std::string{kId};
        return r;
    });
    auto http_client = utest::CreateHttpClient();
    auto http_response = http_client->CreateRequest().get(http_server.GetBaseUrl() + "/test1/typed/id").perform();

    const auto response = typed_client::ParseResponse(*http_response);

    EXPECT_EQ(response.X_Request_Id, ToUuid(kId));
}

UTEST(Parameters, StrongTypedefsAndChronoAreSerialized) {
    std::string received_url;
    std::string received_account_id;
    const utest::HttpServerMock http_server([&](const utest::HttpServerMock::HttpRequest& request) {
        received_url = request.path;
        for (const auto& [name, value] : request.query) {
            received_url += fmt::format(" {}={}", name, value);
        }
        const auto it = request.headers.find(kAccountIdHeader);
        if (it != request.headers.end()) {
            received_account_id = it->second;
        }
        utest::HttpServerMock::HttpResponse r;
        r.response_status = 200;
        return r;
    });
    auto http_client_ptr = utest::CreateHttpClient();
    auto http_request = http_client_ptr->CreateRequest();

    strong_client::Request request;
    request.account = ::test::AccountId{ToUuid(kAccount)};
    request.station = ::test::StationId{ToUuid(kStation)};
    request.accounts = {::test::AccountId{ToUuid(kId)}, ::test::AccountId{ToUuid(kAccount)}};
    request.count = ::test::Count{42};
    request.label = ::test::Label{"hello"};
    request.timeout = std::chrono::milliseconds{1500};
    request.X_Account_Id = ::test::AccountId{ToUuid(kTraceId)};
    strong_client::SerializeRequest(request, http_server.GetBaseUrl(), http_request);
    EXPECT_TRUE(http_request.perform()->IsOk());

    EXPECT_EQ(
        received_url,
        fmt::format(
            "/test1/strong/{} accounts={}%2C{} count=42 label=hello station={} timeout=1500", kAccount, kId, kAccount, kStation
        )
    );
    EXPECT_EQ(received_account_id, kTraceId);
}

UTEST(Parameters, StrongTypedefAndChronoHeadersAreParsed) {
    const utest::HttpServerMock http_server([](const utest::HttpServerMock::HttpRequest&) {
        utest::HttpServerMock::HttpResponse r;
        r.response_status = 200;
        r.headers[std::string{"X-Account-Id"}] = std::string{kAccount};
        r.headers[std::string{"X-Retry-After"}] = "30";
        return r;
    });
    auto http_client = utest::CreateHttpClient();
    auto http_response =
        http_client->CreateRequest().get(http_server.GetBaseUrl() + "/test1/strong/account").perform();

    const auto response = strong_client::ParseResponse(*http_response);

    EXPECT_EQ(response.X_Account_Id, ::test::AccountId{ToUuid(kAccount)});
    EXPECT_EQ(response.X_Retry_After, std::chrono::seconds{30});
}

}  // namespace

USERVER_NAMESPACE_END
