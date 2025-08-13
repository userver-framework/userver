#include <userver/utest/utest.hpp>

#include <clients/parameters/requests.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

namespace client = ::clients::parameters::test1::post;

UTEST(Parameters, CppName) {
    client::Request request;

    // compilation test
    request.myclass = "123";
}

}  // namespace

USERVER_NAMESPACE_END
