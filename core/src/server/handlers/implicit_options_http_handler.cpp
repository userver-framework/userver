#include "implicit_options_http_handler.hpp"

#include <fmt/compile.h>
#include <fmt/format.h>

#include <http/common_headers.hpp>
#include <server/component.hpp>
#include <server/handlers/auth/auth_checker.hpp>
#include <server/handlers/auth/auth_checker_factory.hpp>
#include <server/handlers/auth/handler_auth_config.hpp>
#include <server/http/handler_info_index.hpp>
#include <server/http/handler_methods.hpp>
#include <server/http/http_request_handler.hpp>

namespace server::handlers {
namespace {

ImplicitOptionsHttpHandler::AuthCheckers MakeAuthCheckers(
    const components::ComponentConfig& config,
    const components::ComponentContext& component_context) {
  constexpr auto kAuthCheckers = "auth_checkers";

  if (!config.HasMember(kAuthCheckers)) return {};

  auth::HandlerAuthConfig auth_config(config[kAuthCheckers]);

  const auto& http_server_settings =
      component_context.FindComponent<components::HttpServerSettingsBase>();

  ImplicitOptionsHttpHandler::AuthCheckers checkers;
  for (const auto& type : auth_config.GetTypes()) {
    try {
      const auto& auth_factory = auth::GetAuthCheckerFactory(type);
      auto sp_checker =
          auth_factory(component_context, auth_config,
                       http_server_settings.GetAuthCheckerSettings());
      if (sp_checker) {
        checkers[type] = sp_checker;
        LOG_INFO() << "Loaded " << type
                   << " auth checker for implicit options handler";
      } else
        LOG_ERROR() << "Internal error during creating " << type
                    << " auth checker";
    } catch (const std::exception& err) {
      LOG_ERROR() << "Unable to create " << type << " auth checker "
                  << "for implicit OPTIONS handler, skipping the check: "
                  << err.what();
    }
  }

  return checkers;
}

}  // namespace

ImplicitOptionsHttpHandler::ImplicitOptionsHttpHandler(
    const components::ComponentConfig& config,
    const components::ComponentContext& component_context, bool is_monitor)
    : HttpHandlerBase(config, component_context, is_monitor),
      server_(
          component_context.FindComponent<components::Server>().GetServer()),
      auth_checkers_(MakeAuthCheckers(config, component_context)) {}

std::string ImplicitOptionsHttpHandler::ExtractAllowedMethods(
    const std::string& path) const {
  std::set<std::string> allowed_methods = {
      ToString(http::HttpMethod::kOptions)};

  LOG_DEBUG() << "Requesting OPTIONS for path " << path;

  for (const auto method : http::kHandlerMethods) {
    auto match_result = GetHandlerInfoIndex().MatchRequest(method, path);
    switch (match_result.status) {
      case http::MatchRequestResult::Status::kOk:
        allowed_methods.insert(ToString(method));
        break;
      case http::MatchRequestResult::Status::kHandlerNotFound:
        LOG_ERROR() << "No handlers available for path " << path;
        return ToString(http::HttpMethod::kOptions);
      case http::MatchRequestResult::Status::kMethodNotAllowed:
        break;
    }
  }

  return fmt::format(FMT_COMPILE("{}"), fmt::join(allowed_methods, ", "));
}

const http::HandlerInfoIndex& ImplicitOptionsHttpHandler::GetHandlerInfoIndex()
    const {
  if (handler_info_index_) return *handler_info_index_;

  std::lock_guard lock(handler_info_index_mutex_);

  handler_info_index_ =
      &server_.GetHttpRequestHandler(IsMonitor()).GetHandlerInfoIndex();
  return *handler_info_index_;
}

const std::string& ImplicitOptionsHttpHandler::HandlerName() const {
  static const std::string kHandlerName = kName;
  return kHandlerName;
}

std::string ImplicitOptionsHttpHandler::HandleRequestThrow(
    const server::http::HttpRequest& request,
    server::request::RequestContext& context) const {
  auto& response = request.GetHttpResponse();

  response.SetHeader(::http::headers::kAllow,
                     ExtractAllowedMethods(request.GetRequestPath()));

  if (request.HasHeader(::http::headers::kXYaTaxiAllowAuthRequest)) {
    constexpr auto kUnknownChecker = "unknown checker";
    std::optional<std::string> check_status;

    const auto& check_type =
        request.GetHeader(::http::headers::kXYaTaxiAllowAuthRequest);
    const auto it = auth_checkers_.find(check_type);

    if (it != auth_checkers_.end() && it->second) {
      auto check_result = it->second->CheckAuth(request, context);
      check_status = auth::GetDefaultReasonForStatus(check_result.status);
    } else {
      LOG_WARNING() << "Auth checker for '" << check_type
                    << "' not found, skipping";
    }

    response.SetHeader(::http::headers::kXYaTaxiAllowAuthResponse,
                       check_status.value_or(kUnknownChecker));
    response.SetHeader(::http::headers::kAccessControlAllowHeaders,
                       ::http::headers::kXYaTaxiAllowAuthResponse);
  }

  static const std::string kEmpty;
  return kEmpty;
}

}  // namespace server::handlers
