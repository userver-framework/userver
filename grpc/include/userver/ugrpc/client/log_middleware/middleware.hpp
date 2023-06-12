#pragma once

/// @file userver/ugrpc/client/log_middleware/middleware.hpp
/// @brief @copybrief ugrpc::client::log_middleware::Middleware

#include <userver/ugrpc/client/middleware_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::log_middleware {

/// @brief middleware for RPC handler logging settings
class Middleware final : public MiddlewareBase {
 public:
  struct Settings {
    size_t max_msg_size;  ///< Max gRPC message size, the rest will be truncated
    logging::Level log_level;  ///< gRPC message logging level
  };

  explicit Middleware(const Settings& settings);

  void Handle(MiddlewareCallContext& context) const override;

 private:
  Settings settings_;
};

/// @cond
class MiddlewareFactory final : public MiddlewareFactoryBase {
 public:
  explicit MiddlewareFactory(const Middleware::Settings& settings);

  std::shared_ptr<const MiddlewareBase> GetMiddleware(
      std::string_view client_name) const override;

 private:
  Middleware::Settings settings_;
};
/// @endcond

}  // namespace ugrpc::client::log_middleware

USERVER_NAMESPACE_END
