#pragma once

#include <userver/ugrpc/server/middlewares/base.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::middlewares::log {

class Middleware final : public MiddlewareBase {
 public:
  struct Settings {
    size_t max_msg_size{};
    USERVER_NAMESPACE::logging::Level msg_log_level{};
    std::optional<USERVER_NAMESPACE::logging::Level> local_log_level;
  };

  explicit Middleware(const Settings& settings);

  void Handle(MiddlewareCallContext& context) const override;

 private:
  Settings settings_;
};

}  // namespace ugrpc::server::middlewares::log

USERVER_NAMESPACE_END
