#include <server/http/wildcard_path_index.hpp>

#include <stdexcept>

#include <boost/algorithm/string/split.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::http::impl {
namespace {

const std::string kAnySuffixMark{"*"};

constexpr char kWildcardStart = '{';
constexpr char kWildcardFinish = '}';

std::vector<std::string> SplitBySlash(const std::string& path) {
  std::vector<std::string> path_vec;
  boost::split(path_vec, path, [](char c) { return c == '/'; });
  return path_vec;
}

std::string ExtractWildcardName(const std::string& str) {
  if (str.empty() || str.front() != kWildcardStart ||
      str.back() != kWildcardFinish) {
    throw std::runtime_error("Incorrect wildcard '" + str + '\'');
  }

  return str.substr(1, str.size() - 2);
}

bool GetFromHandlerMethodIndex(const WildcardPathIndex::Node& node,
                               HttpMethod method,
                               const std::vector<std::string>& path,
                               MatchRequestResult& match_result,
                               bool limit_path_length) {
  const auto& index_map = node.handler_method_index_map;
  decltype(index_map.begin()) it;
  if (limit_path_length) {
    it = index_map.find(path.size());
  } else {
    it = index_map.upper_bound(path.size());
    if (it == index_map.begin()) return false;
    --it;
  }
  if (it == index_map.end()) return false;
  const auto& handler_method_index = it->second;

  const auto* handler_info_data =
      handler_method_index.GetHandlerInfoData(method);
  if (!handler_info_data) {
    match_result.status = MatchRequestResult::Status::kMethodNotAllowed;
    return false;
  }

  match_result.handler_info = &handler_info_data->handler_info;
  for (const auto& arg : handler_info_data->wildcards) {
    if (arg.index > path.size())
      throw std::logic_error(
          "matched path from handler has length greater than path from "
          "request");
    match_result.args_from_path.emplace_back(
        arg.name, arg.index == path.size() ? std::string{} : path[arg.index]);
  }
  match_result.status = MatchRequestResult::Status::kOk;
  return true;
}

}  // namespace

bool HasWildcardSpecificSymbols(const std::string& path) {
  return path.find(kWildcardStart) != std::string::npos ||
         path.find(kWildcardFinish) != std::string::npos;
}

void WildcardPathIndex::AddHandler(const handlers::HttpHandlerBase& handler,
                                   engine::TaskProcessor& task_processor) {
  const auto& path = std::get<std::string>(handler.GetConfig().path);
  AddHandler(path, handler, task_processor);

  auto url_trailing_slash = handler.GetConfig().url_trailing_slash;
  if (url_trailing_slash == handlers::UrlTrailingSlashOption::kBoth &&
      !path.empty()) {
    if (path.back() == '/') {
      if (path.size() > 1) {
        if (path[path.size() - 2] == '/')
          throw std::runtime_error(
              "can't use 'url_trailing_slash' option with path ends with '//'");
        AddHandler(path.substr(0, path.size() - 1), handler, task_processor);
      }
    } else if (path.back() == '*') {
      if (path.size() > 1 && path[path.size() - 2] == '/') {
        // ends with '/*' but not with '//*'
        if (path.size() > 2 && path[path.size() - 3] == '/')
          throw std::runtime_error(
              "can't use 'url_trailing_slash' option with path ends with "
              "'//*'");
        AddHandler(path.substr(0, path.size() - 2), handler, task_processor);
      } else {
        throw std::runtime_error("incorrect path: '" + path +
                                 "': trailing '*' allowed after '/' only");
      }
    } else {
      AddHandler(path + '/', handler, task_processor);
    }
  }
}

bool WildcardPathIndex::MatchRequest(HttpMethod method, const std::string& path,
                                     MatchRequestResult& match_result) const {
  return MatchRequest(root_, method, SplitBySlash(path), path.size(),
                      match_result);
}

void WildcardPathIndex::AddHandler(const std::string& path,
                                   const handlers::HttpHandlerBase& handler,
                                   engine::TaskProcessor& task_processor) {
  auto path_vec = SplitBySlash(path);
  std::vector<PathItem> path_fixed_items;
  std::vector<PathItem> path_wildcards;
  std::unordered_set<std::string> wildcard_names;
  try {
    for (size_t i = 0; i < path_vec.size(); i++) {
      if (!HasWildcardSpecificSymbols(path_vec[i])) {
        path_fixed_items.emplace_back(
            ExtractFixedPathItem(i, std::move(path_vec[i])));
      } else {
        path_wildcards.emplace_back(
            ExtractWildcardPathItem(i, path_vec[i], wildcard_names));
      }
    }
  } catch (const std::exception& ex) {
    throw std::runtime_error("Failed to process handler path '" + path +
                             "': " + ex.what());
  }
  AddPath(handler, task_processor, std::move(path_fixed_items),
          std::move(path_wildcards));
}

void WildcardPathIndex::AddPath(const handlers::HttpHandlerBase& handler,
                                engine::TaskProcessor& task_processor,
                                std::vector<PathItem>&& fixed_path,
                                std::vector<PathItem> wildcards) {
  size_t length = fixed_path.size() + wildcards.size();
  Node* cur = &root_;
  for (auto& path_item : std::move(fixed_path)) {
    cur = &cur->next[path_item.index][std::move(path_item.name)];
  }
  cur->handler_method_index_map[length].AddHandler(handler, task_processor,
                                                   std::move(wildcards));
}

bool WildcardPathIndex::MatchRequest(const Node& node, HttpMethod method,
                                     const std::vector<std::string>& path,
                                     size_t path_string_length,
                                     MatchRequestResult& match_result) const {
  for (const auto& next_item : node.next) {
    if (next_item.first >= path.size()) break;
    auto it = next_item.second.find(path[next_item.first]);
    if (it != next_item.second.end()) {
      if (MatchRequest(it->second, method, path, path_string_length,
                       match_result))
        return true;
    }
  }

  // check for match without '*'
  if (GetFromHandlerMethodIndex(node, method, path, match_result, true)) {
    match_result.matched_path_length = path_string_length;
    return true;
  }

  // check "/some/.../path/*"
  auto node_next_it = node.next.lower_bound(path.size());
  while (node_next_it != node.next.begin()) {
    --node_next_it;
    auto it = node_next_it->second.find(kAnySuffixMark);
    if (it != node_next_it->second.end()) {
      if (GetFromHandlerMethodIndex(it->second, method, path, match_result,
                                    false)) {
        size_t asterisk_pos = node_next_it->first;
        match_result.matched_path_length = asterisk_pos;
        for (size_t i = 0; i < asterisk_pos; i++) {
          match_result.matched_path_length += path[i].size();
        }
        for (size_t i = asterisk_pos; i < path.size(); i++) {
          match_result.args_from_path.emplace_back(std::string{}, path[i]);
        }
        return true;
      }
    }
  }

  return false;
}

PathItem WildcardPathIndex::ExtractFixedPathItem(size_t index,
                                                 std::string&& path_elem) {
  return PathItem{index, std::move(path_elem)};
}

PathItem WildcardPathIndex::ExtractWildcardPathItem(
    size_t index, const std::string& path_elem,
    std::unordered_set<std::string>& wildcard_names) {
  auto wildcard_name = ExtractWildcardName(path_elem);
  if (!wildcard_name.empty()) {
    auto res = wildcard_names.emplace(wildcard_name);
    if (!res.second) {
      throw std::runtime_error("duplicate wildcard name: '" + wildcard_name +
                               '\'');
    }
  }
  return PathItem{index, std::move(wildcard_name)};
}

}  // namespace server::http::impl

USERVER_NAMESPACE_END
