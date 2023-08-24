#pragma once

#include <array>
#include <list>
#include <string>
#include <vector>

#include <userver/engine/task/task_processor_fwd.hpp>

#include <server/http/handler_info_index.hpp>
#include <server/http/handler_methods.hpp>
#include <userver/server/handlers/http_handler_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::http::impl {

struct PathItem final {
  PathItem() = default;
  PathItem(size_t index, std::string name)
      : index(index), name(std::move(name)) {}

  size_t index{0};
  std::string name;
};

class HandlerMethodIndex final {
 public:
  struct HandlerInfoData {
    HandlerInfoData(engine::TaskProcessor& task_processor,
                    const handlers::HttpHandlerBase& handler,
                    std::vector<PathItem> wildcards)
        : handler_info(task_processor, handler),
          wildcards(std::move(wildcards)) {}

    HandlerInfo handler_info;
    std::vector<PathItem> wildcards;
  };

  void AddHandler(const handlers::HttpHandlerBase& handler,
                  engine::TaskProcessor& task_processor,
                  std::vector<PathItem> wildcards);
  [[nodiscard]] const HandlerInfoData* GetHandlerInfoData(
      HttpMethod method) const;

 private:
  void AddHandlerInfoData(HttpMethod method,
                          HandlerInfoData& handler_info_data);

  [[nodiscard]] bool IsAllowedMethod(size_t method_index) const;

  std::list<HandlerInfoData> handler_info_holder_;
  std::array<const HandlerInfoData*, kHandlerMethodsMax + 1> pmethods_{};
};

}  // namespace server::http::impl

USERVER_NAMESPACE_END
