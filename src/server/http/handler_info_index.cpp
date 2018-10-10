#include "handler_info_index.hpp"

#include <stdexcept>

#include <logging/logger.hpp>

namespace server {
namespace http {

void HandlerInfoIndex::AddHandler(const handlers::HandlerBase& handler,
                                  engine::TaskProcessor& task_processor) {
  auto is_inserted =
      handler_infos_
          .emplace(std::piecewise_construct, std::tie(handler.GetConfig().path),
                   std::tie(task_processor, handler))
          .second;

  if (!is_inserted) {
    throw std::runtime_error("duplicate handler paths: " +
                             handler.GetConfig().path);
  }
}

bool HandlerInfoIndex::GetHandlerInfo(const std::string& path,
                                      HandlerInfo& handler_info) const {
  auto it = handler_infos_.find(path);
  if (it == handler_infos_.end()) {
    std::string path_prefix = path;
    for (size_t pos = path_prefix.rfind('/'); pos != std::string::npos;
         pos = path_prefix.rfind('/')) {
      path_prefix.resize(pos + 1);
      path_prefix += '*';
      it = handler_infos_.find(path_prefix);
      if (it != handler_infos_.end()) {
        handler_info = it->second;
        handler_info.matched_path_length = pos + 1;
        return true;
      }
      path_prefix.resize(pos);  // remove trailing "/*"
    }
    return false;
  }
  handler_info = it->second;
  handler_info.matched_path_length = path.size();
  return true;
}

}  // namespace http
}  // namespace server
