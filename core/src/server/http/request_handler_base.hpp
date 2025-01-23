#pragma once

#include <server/http/handler_info_index.hpp>
#include <userver/logging/logger.hpp>
#include <userver/server/handlers/handler_base.hpp>
#include <userver/server/http/http_request.hpp>

#include <userver/engine/task/task_with_result.hpp>
#include <userver/logging/impl/logger_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::http {

class RequestHandlerBase {
public:
    virtual ~RequestHandlerBase() noexcept;

    virtual engine::TaskWithResult<void> StartRequestTask(std::shared_ptr<http::HttpRequest> request) const = 0;

    virtual const HandlerInfoIndex& GetHandlerInfoIndex() const = 0;

    virtual const logging::TextLoggerPtr& LoggerAccess() const noexcept = 0;
    virtual const logging::TextLoggerPtr& LoggerAccessTskv() const noexcept = 0;
};

}  // namespace server::http

USERVER_NAMESPACE_END
