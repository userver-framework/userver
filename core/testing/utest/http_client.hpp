#pragma once

#include <memory>

namespace clients {
namespace http {
class Client;
}
}  // namespace clients

namespace utest {

std::shared_ptr<clients::http::Client> CreateHttpClient();

}
