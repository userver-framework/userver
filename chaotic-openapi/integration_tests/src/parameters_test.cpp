#include <userver/utest/utest.hpp>

#include <string>
#include <string_view>

#include <boost/uuid/string_generator.hpp>
#include <fmt/format.h>

#include <clients/parameters/requests.hpp>
#include <clients/parameters/responses.hpp>
#include <userver/clients/http/client.hpp>
#include <userver/http/predefined_header.hpp>
#include <userver/utest/http_client.hpp>
#include <userver/utest/http_server_mock.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

namespace client = ::clients::parameters::test1::post;
namespace typed_client = ::clients::parameters::test1_typed_id::get;

constexpr std::string_view kId = "6ba7b810-9dad-11d1-80b4-00c04fd430c8";
constexpr std::string_view kAccount = "6ba7b811-9dad-11d1-80b4-00c04fd430c8";
constexpr std::string_view kStation = "6ba7b812-9dad-11d1-80b4-00c04fd430c8";
constexpr std::string_view kTraceId = "6ba7b813-9dad-11d1-80b4-00c04fd430c8";
constexpr http::headers::PredefinedHeader kTraceIdHeader{"X-Trace-Id"};

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
    http_request.perform();

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

}  // namespace

USERVER_NAMESPACE_END
