#pragma once

#include <memory>
#include <string>
#include <vector>

#include <boost/optional.hpp>

#include <engine/task/task_processor.hpp>
#include <server/handlers/http_handler_base.hpp>
#include <server/http/http_method.hpp>

namespace server {
namespace http {

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
      : handler_info(handler_info) {}
  boost::optional<const HandlerInfo&> handler_info;

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
  MatchRequestResult MatchRequest(HttpMethod method,
                                  const std::string& path) const;

 private:
  class HandlerInfoIndexImpl;

  std::unique_ptr<HandlerInfoIndexImpl> impl_;
};

}  // namespace http
}  // namespace server
