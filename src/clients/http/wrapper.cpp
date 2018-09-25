#include <clients/http/wrapper.hpp>

#include <clients/http/client.hpp>
#include <clients/http/response_future.hpp>
#include <curl-ev/easy.hpp>

namespace clients {
namespace http {

EasyWrapper::EasyWrapper(std::shared_ptr<curl::easy> easy,
                         std::weak_ptr<Client> client)
    : easy_(std::move(easy)), client_(std::move(client)) {}

EasyWrapper::~EasyWrapper() {
  auto client = client_.lock();
  if (client) client->PushIdleEasy(std::move(easy_));
}

curl::easy& EasyWrapper::Easy() { return *easy_; }

}  // namespace http
}  // namespace clients
