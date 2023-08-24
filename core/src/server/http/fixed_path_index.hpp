#pragma once

#include <string>
#include <unordered_map>

#include <userver/engine/task/task_processor_fwd.hpp>

#include <server/http/handler_info_index.hpp>
#include <server/http/handler_method_index.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/http/http_method.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::http::impl {

class FixedPathIndex final {
 public:
  void AddHandler(const handlers::HttpHandlerBase& handler,
                  engine::TaskProcessor& task_processor);
  bool MatchRequest(HttpMethod method, const std::string& path,
                    MatchRequestResult& match_result) const;

 private:
  void AddHandler(std::string path, const handlers::HttpHandlerBase& handler,
                  engine::TaskProcessor& task_processor);

  std::unordered_map<std::string, HandlerMethodIndex> handler_method_index_map_;
};

}  // namespace server::http::impl

USERVER_NAMESPACE_END
