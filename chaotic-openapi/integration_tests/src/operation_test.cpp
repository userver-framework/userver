#include <userver/utest/utest.hpp>

#include <userver/utest/http_client.hpp>

#include <clients/operation/client_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

UTEST(Operation, OperationId) {
    auto http_client = utest::CreateHttpClient();
    ::clients::operation::ClientImpl client(
        {
            "",
        },
        *http_client
    );

    if (false) {
        // Dead code intentionally
        [[maybe_unused]] ::clients::operation::mytest::Response200 response = client.MyTest({});
    }
}

}  // namespace

USERVER_NAMESPACE_END
