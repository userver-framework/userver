#pragma once

#include <memory>

#include <userver/server/request/request_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::request {

class RequestConstructor {
 public:
  RequestConstructor() = default;
  virtual ~RequestConstructor() = default;

  virtual std::shared_ptr<RequestBase> Finalize() = 0;
};

}  // namespace server::request

USERVER_NAMESPACE_END
