#pragma once

/// @file server/handlers/jemalloc.hpp
/// @brief @copybrief server::handlers::Jemalloc

#include <logging/level.hpp>

#include <server/handlers/http_handler_base.hpp>

namespace server::handlers {

// clang-format off

/// @ingroup userver_http_handlers
///
/// @brief Handler that controlls the jemalloc allocator.
///
/// The component has no service configuration except the
/// @ref userver_http_handlers "common handler options".
///
/// ## Static configuration example:
///
/// @snippet components/common_server_component_list_test.cpp  Sample handler jemalloc component config
///
/// ## Scheme
/// Set an URL path argument `command` to one of the following values:
/// * `stat` - to get jemalloc stats
/// * `enable` - to start memory profiling
/// * `disable` - to stop memory profiling
/// * `dump` - to get jemalloc profiling dump

// clang-format on

class Jemalloc final : public HttpHandlerBase {
 public:
  Jemalloc(const components::ComponentConfig& config,
           const components::ComponentContext& component_context);

  static constexpr const char* kName = "handler-jemalloc";

  const std::string& HandlerName() const override;
  std::string HandleRequestThrow(const http::HttpRequest& request,
                                 request::RequestContext&) const override;
};

}  // namespace server::handlers
