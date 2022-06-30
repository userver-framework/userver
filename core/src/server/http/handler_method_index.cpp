#include <server/http/handler_method_index.hpp>

#include <stdexcept>

USERVER_NAMESPACE_BEGIN

namespace server::http::impl {

void HandlerMethodIndex::AddHandler(const handlers::HttpHandlerBase& handler,
                                    engine::TaskProcessor& task_processor,
                                    std::vector<PathItem> wildcards) {
  auto& handler_info_data =
      *handler_info_holder_.emplace(handler_info_holder_.end(), task_processor,
                                    handler, std::move(wildcards));

  for (auto method : handler.GetAllowedMethods()) {
    AddHandlerInfoData(method, handler_info_data);
  }
}

const HandlerMethodIndex::HandlerInfoData*
HandlerMethodIndex::GetHandlerInfoData(HttpMethod method) const {
  auto index = static_cast<size_t>(method);
  if (!IsAllowedMethod(index)) {
    return nullptr;
  }
  return pmethods_[index];
}

void HandlerMethodIndex::AddHandlerInfoData(
    HttpMethod method, HandlerInfoData& handler_info_data) {
  auto index = static_cast<size_t>(method);
  UASSERT(index <= kHandlerMethodsMax);
  if (pmethods_[index]) {
    const auto& new_handler = handler_info_data.handler_info.handler;
    const auto& old_handler = pmethods_[index]->handler_info.handler;
    throw std::runtime_error(
        "duplicate handler method+path: method=" + ToString(method) +
        " path1=" + std::get<std::string>(new_handler.GetConfig().path) +
        " path2=" + std::get<std::string>(old_handler.GetConfig().path));
  }
  pmethods_[index] = &handler_info_data;
}

bool HandlerMethodIndex::IsAllowedMethod(size_t method_index) const {
  return method_index <= kHandlerMethodsMax && pmethods_[method_index];
}

}  // namespace server::http::impl

USERVER_NAMESPACE_END
