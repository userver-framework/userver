#include <clients/http/wrapper.hpp>

#include <clients/http/client.hpp>
#include <clients/http/response_future.hpp>
#include <curl-ev/easy.hpp>

#include <utils/assert.hpp>

namespace clients {
namespace http {

EasyWrapper::EasyWrapper(std::shared_ptr<curl::easy> easy, Client& client)
    : easy_(std::move(easy)), client_(client) {
  client_.IncPending();
}

EasyWrapper::~EasyWrapper() { client_.PushIdleEasy(std::move(easy_)); }

curl::easy& EasyWrapper::Easy() { return *easy_; }

}  // namespace http
}  // namespace clients
