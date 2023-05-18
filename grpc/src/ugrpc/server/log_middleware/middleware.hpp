#pragma once

#include <userver/ugrpc/server/middleware_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::log_middleware {

class Middleware final : public MiddlewareBase {
 public:
  struct Settings {
    size_t max_msg_size;
    logging::Level msg_log_level;
    std::optional<logging::Level> local_log_level;
  };

  explicit Middleware(const Settings& settings);

  void Handle(MiddlewareCallContext& context) const override;

 private:
  Settings settings_;
};

}  // namespace ugrpc::server::log_middleware

USERVER_NAMESPACE_END
