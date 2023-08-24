#pragma once

#include <userver/ugrpc/client/middlewares/base.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::middlewares::log {

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

}  // namespace ugrpc::client::middlewares::log

USERVER_NAMESPACE_END
