#pragma once

#include <string_view>

#include <userver/server/handlers/auth/auth_checker_factory.hpp>
#include <userver/utest/using_namespace_userver.hpp>

namespace samples::auth {

class CheckerFactory final : public server::handlers::auth::AuthCheckerFactoryBase {
public:
    static constexpr std::string_view kAuthType = "bearer";

    explicit CheckerFactory(const components::ComponentContext& context);

    server::handlers::auth::AuthCheckerBasePtr MakeAuthChecker(
        const server::handlers::auth::HandlerAuthConfig& auth_config
    ) const override;
};

}  // namespace samples::auth