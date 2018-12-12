#pragma once

#include <string>

#include <engine/task/task_processor.hpp>
#include <server/handlers/http_handler_base.hpp>
#include <server/http/http_method.hpp>

namespace server {
namespace http {

struct HandlerInfo {
  HandlerInfo() = default;
  HandlerInfo(engine::TaskProcessor& task_processor,
              const handlers::HttpHandlerBase& handler)
      : task_processor(&task_processor), handler(&handler) {}

  engine::TaskProcessor* task_processor = nullptr;
  const handlers::HttpHandlerBase* handler = nullptr;
  size_t matched_path_length = 0;
};

class HandlerInfoIndex {
 public:
  HandlerInfoIndex();
  ~HandlerInfoIndex();

  void AddHandler(const handlers::HttpHandlerBase& handler,
                  engine::TaskProcessor& task_processor);
  HandlerInfo GetHandlerInfo(HttpMethod method, const std::string& path) const;

 private:
  class HandlerInfoIndexImpl;

  std::unique_ptr<HandlerInfoIndexImpl> impl_;
};

}  // namespace http
}  // namespace server
