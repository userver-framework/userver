#pragma once

#include <memory>

namespace curl {
class easy;
}  // namespace curl

namespace clients {
namespace http {

class Client;

class EasyWrapper final {
 public:
  EasyWrapper(std::unique_ptr<curl::easy> easy, Client& client);

  EasyWrapper(const EasyWrapper&) = delete;
  EasyWrapper(EasyWrapper&&) = delete;
  EasyWrapper& operator=(const EasyWrapper&) = delete;
  EasyWrapper& operator=(EasyWrapper&&) = delete;

  ~EasyWrapper();

  curl::easy& Easy();

 private:
  std::unique_ptr<curl::easy> easy_;
  Client& client_;
};

}  // namespace http
}  // namespace clients
