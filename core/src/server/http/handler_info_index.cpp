#include "handler_info_index.hpp"

#include <array>
#include <deque>
#include <stdexcept>

#include <logging/logger.hpp>

#include "handler_methods.hpp"

namespace server {
namespace http {

namespace {

class HandlerMethodIndex {
 public:
  HandlerMethodIndex();

  void AddHandler(const handlers::HttpHandlerBase& handler,
                  engine::TaskProcessor& task_processor);
  bool GetHandlerInfo(HttpMethod method, HandlerInfo& handler_info) const;

 private:
  void AddHandlerInfo(HttpMethod method, HandlerInfo& handler_info);

  std::deque<HandlerInfo> handler_info_holder_;
  std::array<HandlerInfo*, kHandlerMethodsMax + 1> pmethods_{};
};

HandlerMethodIndex::HandlerMethodIndex() { pmethods_.fill(nullptr); }

void HandlerMethodIndex::AddHandler(const handlers::HttpHandlerBase& handler,
                                    engine::TaskProcessor& task_processor) {
  auto& handler_info = *handler_info_holder_.emplace(handler_info_holder_.end(),
                                                     task_processor, handler);

  for (auto method : handler.GetAllowedMethods()) {
    AddHandlerInfo(method, handler_info);
  }
}

void HandlerMethodIndex::AddHandlerInfo(HttpMethod method,
                                        HandlerInfo& handler_info) {
  auto index = static_cast<size_t>(method);
  UASSERT(index <= kHandlerMethodsMax);
  if (pmethods_[index])
    throw std::runtime_error(
        "duplicate handler method+path: " + ToString(method) + " " +
        handler_info.handler->GetConfig().path);
  pmethods_[index] = &handler_info;
}

bool HandlerMethodIndex::GetHandlerInfo(HttpMethod method,
                                        HandlerInfo& handler_info) const {
  auto index = static_cast<size_t>(method);
  if (index > kHandlerMethodsMax) return false;
  if (!pmethods_[index]) return false;
  handler_info = *pmethods_[index];
  return true;
}

}  // namespace

class HandlerInfoIndex::HandlerInfoIndexImpl {
 public:
  void AddHandler(const handlers::HttpHandlerBase& handler,
                  engine::TaskProcessor& task_processor);
  HandlerInfo GetHandlerInfo(HttpMethod method, const std::string& path) const;

 private:
  std::unordered_map<std::string, HandlerMethodIndex> handler_infos_;
};

void HandlerInfoIndex::HandlerInfoIndexImpl::AddHandler(
    const handlers::HttpHandlerBase& handler,
    engine::TaskProcessor& task_processor) {
  handler_infos_[handler.GetConfig().path].AddHandler(handler, task_processor);
}

HandlerInfo HandlerInfoIndex::HandlerInfoIndexImpl::GetHandlerInfo(
    HttpMethod method, const std::string& path) const {
  HandlerInfo handler_info;
  auto it = handler_infos_.find(path);
  if (it == handler_infos_.end() ||
      !it->second.GetHandlerInfo(method, handler_info)) {
    std::string path_prefix = path;
    for (size_t pos = path_prefix.rfind('/'); pos != std::string::npos;
         pos = path_prefix.rfind('/')) {
      path_prefix.resize(pos + 1);
      path_prefix += '*';
      it = handler_infos_.find(path_prefix);
      if (it != handler_infos_.end() &&
          it->second.GetHandlerInfo(method, handler_info)) {
        handler_info.matched_path_length = pos + 1;
        return handler_info;
      }
      path_prefix.resize(pos);  // remove trailing "/*"
    }
    return handler_info;
  }
  handler_info.matched_path_length = path.size();
  return handler_info;
}

HandlerInfoIndex::HandlerInfoIndex()
    : impl_(std::make_unique<HandlerInfoIndexImpl>()) {}

HandlerInfoIndex::~HandlerInfoIndex() = default;

void HandlerInfoIndex::AddHandler(const handlers::HttpHandlerBase& handler,
                                  engine::TaskProcessor& task_processor) {
  impl_->AddHandler(handler, task_processor);
}

HandlerInfo HandlerInfoIndex::GetHandlerInfo(HttpMethod method,
                                             const std::string& path) const {
  return impl_->GetHandlerInfo(method, path);
}

}  // namespace http
}  // namespace server
