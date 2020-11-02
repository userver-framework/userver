#pragma once

/// @file server/handlers/handler_base.hpp
/// @brief @copybrief server::handlers::HandlerBase

#include <components/component_config.hpp>
#include <components/component_context.hpp>
#include <components/loggable_component_base.hpp>
#include <server/handlers/exceptions.hpp>
#include <server/handlers/handler_config.hpp>
#include <server/request/request_base.hpp>
#include <server/request/request_context.hpp>

namespace server::handlers {

/// Base class for request handlers.
class HandlerBase : public components::LoggableComponentBase {
 public:
  HandlerBase(const components::ComponentConfig& config,
              const components::ComponentContext& component_context,
              bool is_monitor = false);
  ~HandlerBase() noexcept override = default;

  /// Parses request, executes processing routines, and fills response
  /// accordingly. Does not throw.
  virtual void HandleRequest(request::RequestBase& request,
                             request::RequestContext& context) const = 0;

  /// Produces response to a request unrecognized by the protocol based on
  /// provided generic response. Does not throw.
  virtual void ReportMalformedRequest(request::RequestBase&) const {}

  /// Returns whether this is a monitoring handler.
  bool IsMonitor() const { return is_monitor_; }

  /// Returns handler config.
  const HandlerConfig& GetConfig() const;

  /// Returns whether the handler is enabled
  bool IsEnabled() const { return is_enabled_; }

 protected:
  // Pull the type names in the handler's scope to shorten throwing code
  using HandlerErrorCode = handlers::HandlerErrorCode;
  using InternalMessage = handlers::InternalMessage;
  using ExternalBody = handlers::ExternalBody;

  using ClientError = handlers::ClientError;
  using InternalServerError = handlers::InternalServerError;

 private:
  HandlerConfig config_;
  bool is_monitor_;
  bool is_enabled_;
};

}  // namespace server::handlers
