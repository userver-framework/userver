#include "wrapper.hpp"
#include "client.hpp"

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
