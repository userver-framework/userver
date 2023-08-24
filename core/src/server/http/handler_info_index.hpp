#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/server/handlers/fallback_handlers.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/http/http_method.hpp>
#include <userver/utils/not_null.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::http {

struct HandlerInfo {
  HandlerInfo(engine::TaskProcessor& task_processor,
              const handlers::HttpHandlerBase& handler)
      : task_processor(task_processor), handler(handler) {}

  engine::TaskProcessor& task_processor;
  const handlers::HttpHandlerBase& handler;
};

struct MatchRequestResult {
  enum class Status { kHandlerNotFound, kMethodNotAllowed, kOk };

  MatchRequestResult() = default;
  explicit MatchRequestResult(const HandlerInfo& handler_info)
      : handler_info(&handler_info) {}

  const HandlerInfo* handler_info = nullptr;
  size_t matched_path_length = 0;
  Status status = Status::kHandlerNotFound;
  std::vector<std::pair<std::string, std::string>> args_from_path;
};

class HandlerInfoIndex final {
 public:
  HandlerInfoIndex();
  ~HandlerInfoIndex();

  void AddHandler(const handlers::HttpHandlerBase& handler,
                  engine::TaskProcessor& task_processor);

  using HandlerList =
      std::vector<utils::NotNull<const handlers::HttpHandlerBase*>>;
  const HandlerList& GetHandlers() const;

  const HandlerInfo* GetFallbackHandler(handlers::FallbackHandler) const;

  MatchRequestResult MatchRequest(HttpMethod method,
                                  const std::string& path) const;

 private:
  class HandlerInfoIndexImpl;

  std::unique_ptr<HandlerInfoIndexImpl> impl_;
};

}  // namespace server::http

USERVER_NAMESPACE_END
