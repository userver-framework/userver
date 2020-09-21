#include <clients/http/easy_wrapper.hpp>

#include <clients/http/client.hpp>
#include <clients/http/response_future.hpp>
#include <utils/assert.hpp>

namespace clients::http::impl {

EasyWrapper::EasyWrapper(std::shared_ptr<curl::easy> easy, Client& client)
    : easy_(std::move(easy)), client_(client) {
  client_.IncPending();
}

EasyWrapper::~EasyWrapper() { client_.PushIdleEasy(std::move(easy_)); }

curl::easy& EasyWrapper::Easy() { return *easy_; }

}  // namespace clients::http::impl
