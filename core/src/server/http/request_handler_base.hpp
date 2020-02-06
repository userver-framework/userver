#pragma once

#include <logging/logger.hpp>
#include <server/handlers/handler_base.hpp>
#include <server/http/handler_info_index.hpp>
#include <server/request/request_base.hpp>

#include <engine/task/task_with_result.hpp>

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
