#pragma once

#include <memory>

USERVER_NAMESPACE_BEGIN

namespace clients::http {
class Client;
}  // namespace clients::http

namespace utest {

std::shared_ptr<clients::http::Client> CreateHttpClient();

}  // namespace utest

USERVER_NAMESPACE_END
