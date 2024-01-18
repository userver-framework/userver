#pragma once

/// @file userver/server/handlers/implicit_options.hpp
/// @brief @copybrief server::handlers::ImplicitOptions

#include <userver/engine/mutex.hpp>
#include <userver/server/handlers/http_handler_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace server {
class Server;
}  // namespace server

namespace server::http {
class HandlerInfoIndex;
}  // namespace server::http

namespace server::handlers::auth {
class AuthCheckerBase;
using AuthCheckerBasePtr = std::shared_ptr<AuthCheckerBase>;
}  // namespace server::handlers::auth

namespace server::handlers {

// clang-format off
/// @ingroup userver_components userver_http_handlers
///
/// @brief A "magical" handler that will respond to OPTIONS HTTP method for any
/// other handler that cannot handle OPTIONS itself.
///
/// It returns `Allow` header, listing all the methods that are supported
/// for the given HTTP path, including OPTIONS itself.
///
/// ## Auth checker testing
///
/// In addition to the above RFC behavior, an optional extension is supported.
/// If `auth_checkers` static config option is specified,
/// http::headers::kXYaTaxiAllowAuthRequest header can be passed containing
/// the checker key in the static config.
///
/// The request will then be passed as-is to the auth checker.
/// The result of the check will be returned in
/// http::headers::kXYaTaxiAllowAuthResponse header. The following responses are
/// possible:
///
/// * `unknown checker` if the checker is not found
/// * `OK` if the check succeeds
/// * an unspecified error message if the check fails
///
/// ## Static options
///
/// The component has no service configuration except the
/// @ref userver_http_handlers "common handler options".
/// and adds the following ones:
///
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// auth | server::handlers::auth::HandlerAuthConfig authorization config | auth checker testing is disabled
///
/// ## Static configuration example:
///
/// @snippet components/common_server_component_list_test.cpp  Sample handler implicit http options component config
///
/// ## Scheme
/// Provide an optional query parameter `body` to get the bodies of all the
/// in-flight requests.
// clang-format on
class ImplicitOptions /*non-final*/ : public HttpHandlerBase {
 public:
  /// @ingroup userver_component_names
  /// @brief The default name of server::handlers::ImplicitOptions component
  static constexpr std::string_view kName = "handler-implicit-http-options";

  ImplicitOptions(const components::ComponentConfig& config,
                  const components::ComponentContext& component_context,
                  bool is_monitor = false);

  ~ImplicitOptions() override;

  std::string HandleRequestThrow(
      const server::http::HttpRequest& request,
      server::request::RequestContext& context) const override;

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  using AuthCheckers =
      std::unordered_map<std::string, auth::AuthCheckerBasePtr>;

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
    components::kHasValidate<server::handlers::ImplicitOptions> = true;

USERVER_NAMESPACE_END
