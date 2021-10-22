#pragma once

#include <memory>

#include <curl-ev/easy.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http {
class Client;
}  // namespace clients::http

namespace clients::http::impl {

class EasyWrapper final {
 public:
  EasyWrapper(std::shared_ptr<curl::easy>&& easy, Client& client);

  EasyWrapper(const EasyWrapper&) = delete;
  EasyWrapper(EasyWrapper&&) = delete;
  EasyWrapper& operator=(const EasyWrapper&) = delete;
  EasyWrapper& operator=(EasyWrapper&&) = delete;

  ~EasyWrapper();

  curl::easy& Easy();

 private:
  std::shared_ptr<curl::easy> easy_;
  Client& client_;
};

}  // namespace clients::http::impl

USERVER_NAMESPACE_END
