#include <userver/utest/utest.hpp>

#include <clients/parameters/requests.hpp>
#include <userver/clients/http/client.hpp>
#include <userver/utest/http_client.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

namespace client = ::clients::parameters::test1::post;

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

}  // namespace

USERVER_NAMESPACE_END
