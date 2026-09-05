#include <userver/utest/utest.hpp>

#include <userver/clients/http/client.hpp>
#include <userver/utest/http_client.hpp>
#include <userver/utest/http_server_mock.hpp>

#include <clients/swagger_form_data/requests.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

namespace client = ::clients::swagger_form_data::form_urlencoded::post;

// A swagger "in: formData" parameter describes a single form field rather than
// the whole request body, so it must be serialized as a field of the form.
UTEST(SwaggerFormData, Urlencoded) {
    std::string body;
    const utest::HttpServerMock http_server([&body](const utest::HttpServerMock::HttpRequest& request) {
        body = request.body;
        return utest::HttpServerMock::HttpResponse{};
    });

    auto http_client_ptr = utest::CreateHttpClient();
    auto request = http_client_ptr->CreateRequest();
    client::SerializeRequest(client::Request{{"abc"}}, http_server.GetBaseUrl(), request);
    EXPECT_EQ(request.perform()->status_code(), 200);

    EXPECT_EQ(body, "name=abc");
}

}  // namespace

USERVER_NAMESPACE_END
