#pragma once

#include <string>

#include <engine/task/task_processor.hpp>
#include <server/handlers/handler_base.hpp>

namespace server {
namespace http {

struct HandlerInfo {
  HandlerInfo() = default;
  HandlerInfo(engine::TaskProcessor& task_processor,
              const handlers::HandlerBase& handler)
      : task_processor(&task_processor), handler(&handler) {}

  engine::TaskProcessor* task_processor = nullptr;
  const handlers::HandlerBase* handler = nullptr;
  size_t matched_path_length = 0;
};

class HandlerInfoIndex {
 public:
  void AddHandler(const handlers::HandlerBase& handler,
                  engine::TaskProcessor& task_processor);
  bool GetHandlerInfo(const std::string& path, HandlerInfo& handler_info) const;

 private:
  std::unordered_map<std::string, HandlerInfo> handler_infos_;
};

}  // namespace http
}  // namespace server
