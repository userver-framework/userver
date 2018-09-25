#pragma once

#include <memory>

namespace curl {
class easy;
}  // namespace curl

namespace clients {
namespace http {

class Client;

class EasyWrapper {
 public:
  EasyWrapper(std::shared_ptr<curl::easy> easy, std::weak_ptr<Client> client);

  ~EasyWrapper();

  curl::easy& Easy();

 private:
  std::shared_ptr<curl::easy> easy_;
  std::weak_ptr<Client> client_;
};

}  // namespace http
}  // namespace clients
