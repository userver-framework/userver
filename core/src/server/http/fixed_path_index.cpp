#include <server/http/fixed_path_index.hpp>

#include <stdexcept>

USERVER_NAMESPACE_BEGIN

namespace server::http::impl {

void FixedPathIndex::AddHandler(const handlers::HttpHandlerBase& handler,
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
    } else {
      AddHandler(path + '/', handler, task_processor);
    }
  }
}

void FixedPathIndex::AddHandler(std::string path,
                                const handlers::HttpHandlerBase& handler,
                                engine::TaskProcessor& task_processor) {
  handler_method_index_map_[std::move(path)].AddHandler(handler, task_processor,
                                                        {});
}

bool FixedPathIndex::MatchRequest(HttpMethod method, const std::string& path,
                                  MatchRequestResult& match_result) const {
  auto it = handler_method_index_map_.find(path);
  if (it == handler_method_index_map_.end()) return false;

  const auto* handler_info_data = it->second.GetHandlerInfoData(method);
  if (!handler_info_data) {
    match_result.status = MatchRequestResult::Status::kMethodNotAllowed;
    return false;
  }

  match_result.handler_info = &handler_info_data->handler_info;
  match_result.matched_path_length = path.size();
  match_result.status = MatchRequestResult::Status::kOk;
  return true;
}

}  // namespace server::http::impl

USERVER_NAMESPACE_END
