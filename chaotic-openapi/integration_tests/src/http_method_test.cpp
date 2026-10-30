#include <userver/utest/utest.hpp>

#include <functional>
#include <memory>
#include <string>
#include <string_view>

#include <userver/clients/http/client_core.hpp>
#include <userver/clients/http/request.hpp>
#include <userver/utest/http_client.hpp>
#include <userver/utest/http_server_mock.hpp>

#include <clients/http_method/client_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

namespace http_method = ::clients::http_method;

using clients::http::HttpMethod;

constexpr std::string_view kPayloadJson = R"({"value":"payload"})";

const http_method::Payload kPayload{"payload"};

struct ReceivedRequest {
    HttpMethod method{HttpMethod::kGet};
    std::string path;
    std::string body;
};

class HttpMethodClient {
public:
    HttpMethodClient()
        : http_server_([this](const utest::HttpServerMock::HttpRequest& request) {
              received_ = {request.method, request.path, request.body};
              utest::HttpServerMock::HttpResponse response{};
              response.response_status = 200;
              return response;
          }),
          base_url_(http_server_.GetBaseUrl()),
          http_client_core_(utest::impl::CreateHttpClientCore()),
          client_(MakeConfig(base_url_), *http_client_core_) {}

    ReceivedRequest Send(const std::function<void(http_method::ClientImpl&)>& call) {
        call(client_);
        return received_;
    }

    ReceivedRequest Send(clients::http::Request& request) {
        request.perform();
        return received_;
    }

    const std::string& GetBaseUrl() const { return base_url_; }

private:
    static chaotic::openapi::client::Config MakeConfig(const std::string& base_url) {
        chaotic::openapi::client::Config config;
        config.base_url = base_url;
        return config;
    }

    ReceivedRequest received_;
    utest::HttpServerMock http_server_;
    std::string base_url_;
    std::shared_ptr<clients::http::ClientCore> http_client_core_;
    http_method::ClientImpl client_;
};

}  // namespace

UTEST(HttpMethod, OperationsWithoutBodySendDeclaredMethod) {
    HttpMethodClient client;

    EXPECT_EQ(client.Send([](auto& c) { c.GetWithoutBody(); }).method, HttpMethod::kGet);
    EXPECT_EQ(client.Send([](auto& c) { c.HeadWithoutBody(); }).method, HttpMethod::kHead);
    EXPECT_EQ(client.Send([](auto& c) { c.PostWithoutBody(); }).method, HttpMethod::kPost);
    EXPECT_EQ(client.Send([](auto& c) { c.PutWithoutBody(); }).method, HttpMethod::kPut);
    EXPECT_EQ(client.Send([](auto& c) { c.PatchWithoutBody(); }).method, HttpMethod::kPatch);
    EXPECT_EQ(client.Send([](auto& c) { c.DeleteWithoutBody(); }).method, HttpMethod::kDelete);
    EXPECT_EQ(client.Send([](auto& c) { c.OptionsWithoutBody(); }).method, HttpMethod::kOptions);
}

UTEST(HttpMethod, PostWithoutBodySendsEmptyPost) {
    HttpMethodClient client;

    const auto received = client.Send([](auto& c) { c.PostWithoutBody(); });
    EXPECT_EQ(received.method, HttpMethod::kPost);
    EXPECT_EQ(received.path, "/verb");
    EXPECT_EQ(received.body, "");
}

UTEST(HttpMethod, PostWithOnlyParametersSendsPost) {
    HttpMethodClient client;

    const auto received = client.Send([](auto& c) { c.PostWithParameterOnly({"slug"}); });
    EXPECT_EQ(received.method, HttpMethod::kPost);
    EXPECT_EQ(received.path, "/verb/slug");
}

UTEST(HttpMethod, OperationsWithBodySendDeclaredMethodAndBody) {
    HttpMethodClient client;

    const auto expect_sent = [&](const ReceivedRequest& received, HttpMethod method) {
        EXPECT_EQ(received.method, method);
        EXPECT_EQ(received.body, kPayloadJson);
    };

    expect_sent(client.Send([](auto& c) { c.GetWithBody({kPayload}); }), HttpMethod::kGet);
    expect_sent(client.Send([](auto& c) { c.PostWithBody({kPayload}); }), HttpMethod::kPost);
    expect_sent(client.Send([](auto& c) { c.PutWithBody({kPayload}); }), HttpMethod::kPut);
    expect_sent(client.Send([](auto& c) { c.PatchWithBody({kPayload}); }), HttpMethod::kPatch);
    expect_sent(client.Send([](auto& c) { c.DeleteWithBody({kPayload}); }), HttpMethod::kDelete);
}

UTEST(HttpMethod, SerializeRequestSetsMethod) {
    HttpMethodClient client;
    auto http_client = utest::CreateHttpClient();

    auto request = http_client->CreateRequest();
    http_method::putwithbody::SerializeRequest({kPayload}, client.GetBaseUrl(), request);
    const auto received = client.Send(request);
    EXPECT_EQ(received.method, HttpMethod::kPut);
    EXPECT_EQ(received.body, kPayloadJson);
}

USERVER_NAMESPACE_END
