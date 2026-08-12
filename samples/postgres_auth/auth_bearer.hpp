#pragma once

#include <userver/utest/using_namespace_userver.hpp>  // only for samples

/// [auth checker factory decl]
#include <userver/server/handlers/auth/auth_checker_factory.hpp>

#include "user_info_cache.hpp"

namespace samples::pg {

class CheckerFactory final : public server::handlers::auth::AuthCheckerFactoryBase {
public:
    static constexpr std::string_view kAuthType = "bearer";

    explicit CheckerFactory(const components::ComponentContext& context);

    server::handlers::auth::AuthCheckerBasePtr MakeAuthChecker(
        const server::handlers::auth::HandlerAuthConfig& auth_config
    ) const override;

private:
    AuthCache& auth_cache_;
};

}  // namespace samples::pg
/// [auth checker factory decl]
