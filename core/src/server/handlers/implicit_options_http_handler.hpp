#pragma once

#include <userver/engine/mutex.hpp>

#include <server/handlers/auth/auth_checker.hpp>
#include <userver/server/handlers/http_handler_base.hpp>

USERVER_NAMESPACE_BEGIN

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

  std::string HandleRequestThrow(
      const server::http::HttpRequest& request,
      server::request::RequestContext& context) const override;

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  std::string ExtractAllowedMethods(const std::string& path) const;

  const http::HandlerInfoIndex& GetHandlerInfoIndex() const;

  const Server& server_;
  const AuthCheckers auth_checkers_;

  mutable engine::Mutex handler_info_index_mutex_;
  mutable const http::HandlerInfoIndex* handler_info_index_ = nullptr;
};

}  // namespace server::handlers

template <>
inline constexpr bool
    components::kHasValidate<server::handlers::ImplicitOptionsHttpHandler> =
        true;

USERVER_NAMESPACE_END
