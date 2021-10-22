#pragma once

#include <map>
#include <string>
#include <unordered_set>
#include <vector>

#include <userver/engine/task/task_processor_fwd.hpp>

#include <server/http/handler_info_index.hpp>
#include <server/http/handler_method_index.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/http/http_method.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::http::impl {

bool HasWildcardSpecificSymbols(const std::string& path);

class WildcardPathIndex final {
 public:
  struct Node {
    // ordered by position in path
    std::map<size_t, std::map<std::string, Node>> next;

    // by path length
    std::map<size_t, HandlerMethodIndex> handler_method_index_map;
  };

  void AddHandler(const handlers::HttpHandlerBase& handler,
                  engine::TaskProcessor& task_processor);

  bool MatchRequest(HttpMethod method, const std::string& path,
                    MatchRequestResult& match_result) const;

 private:
  void AddHandler(const std::string& path,
                  const handlers::HttpHandlerBase& handler,
                  engine::TaskProcessor& task_processor);

  void AddPath(const handlers::HttpHandlerBase& handler,
               engine::TaskProcessor& task_processor,
               std::vector<PathItem>&& fixed_path,
               std::vector<PathItem> wildcards);

  bool MatchRequest(const Node& node, HttpMethod method,
                    const std::vector<std::string>& path,
                    size_t path_string_length,
                    MatchRequestResult& match_result) const;

  static PathItem ExtractFixedPathItem(size_t index, std::string&& path_elem);

  static PathItem ExtractWildcardPathItem(
      size_t index, const std::string& path_elem,
      std::unordered_set<std::string>& wildcard_names);

  Node root_;
};

}  // namespace server::http::impl

USERVER_NAMESPACE_END
