#pragma once

#include <memory>

namespace clients::http {
class Client;
}  // namespace clients::http

namespace utest {

std::shared_ptr<clients::http::Client> CreateHttpClient();

}  // namespace utest
