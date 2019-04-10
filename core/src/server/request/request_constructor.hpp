#pragma once

#include <memory>

#include <server/request/request_base.hpp>

namespace server {
namespace request {

class RequestConstructor {
 public:
  RequestConstructor() = default;
  virtual ~RequestConstructor() = default;

  virtual std::shared_ptr<RequestBase> Finalize() = 0;
};

}  // namespace request
}  // namespace server
