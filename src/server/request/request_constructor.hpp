#pragma once

#include <memory>

#include <yandex/taxi/userver/server/request/request_base.hpp>

namespace server {
namespace request {

class RequestConstructor {
 public:
  RequestConstructor() = default;
  virtual ~RequestConstructor() {}

  virtual std::unique_ptr<RequestBase> Finalize() = 0;
};

}  // namespace request
}  // namespace server
