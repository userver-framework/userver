#pragma once

#include <server/http/handler_info_index.hpp>
#include <userver/logging/logger.hpp>
#include <userver/server/handlers/handler_base.hpp>
#include <userver/server/request/request_base.hpp>

#include <userver/engine/task/task_with_result.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::http {

class RequestHandlerBase {
 public:
  virtual ~RequestHandlerBase() noexcept;

  virtual engine::TaskWithResult<void> StartRequestTask(
      std::shared_ptr<request::RequestBase> request) const = 0;

  virtual const HandlerInfoIndex& GetHandlerInfoIndex() const = 0;

  virtual const logging::LoggerPtr& LoggerAccess() const noexcept = 0;
  virtual const logging::LoggerPtr& LoggerAccessTskv() const noexcept = 0;
};

}  // namespace server::http

USERVER_NAMESPACE_END
