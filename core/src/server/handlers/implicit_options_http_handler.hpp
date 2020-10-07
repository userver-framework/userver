#pragma once

#include <server/handlers/auth/auth_checker.hpp>
#include <server/handlers/http_handler_base.hpp>

namespace server {
class Server;
}  // namespace server

namespace server::http {
class HandlerInfoIndex;
}  // namespace server::http

namespace server::handlers {

class ImplicitOptionsHttpHandler final : public HttpHandlerBase {
 public:
  static constexpr auto kName = "handler-implicit-http-options";

  using AuthCheckers =
      std::unordered_map<std::string, auth::AuthCheckerBasePtr>;

  ImplicitOptionsHttpHandler(
      const components::ComponentConfig& config,
      const components::ComponentContext& component_context,
      bool is_monitor = false);

  ~ImplicitOptionsHttpHandler() override = default;

  const std::string& HandlerName() const override;

  std::string HandleRequestThrow(
      const server::http::HttpRequest& request,
      server::request::RequestContext& context) const override;

 private:
  std::string ExtractAllowedMethods(const std::string& path) const;

  const http::HandlerInfoIndex& GetHandlerInfoIndex() const;

  const Server& server_;
  const AuthCheckers auth_checkers_;

  mutable engine::Mutex handler_info_index_mutex_;
  mutable const http::HandlerInfoIndex* handler_info_index_ = nullptr;
};

}  // namespace server::handlers
