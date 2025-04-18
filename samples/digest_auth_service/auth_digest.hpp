#pragma once

#include <userver/utest/using_namespace_userver.hpp>

/// [auth checker factory decl]
#include <userver/server/handlers/auth/auth_checker_factory.hpp>
#include <userver/server/handlers/auth/digest/auth_checker_settings.hpp>
#include <userver/storages/postgres/postgres_fwd.hpp>
#include <userver/storages/secdist/secdist.hpp>

namespace samples::digest_auth {

class CheckerFactory final : public server::handlers::auth::AuthCheckerFactoryBase {
public:
    static constexpr std::string_view kAuthType = "digest";

    explicit CheckerFactory(const components::ComponentContext& context);

    server::handlers::auth::AuthCheckerBasePtr MakeAuthChecker(
        const server::handlers::auth::HandlerAuthConfig& auth_config
    ) const override;

private:
    const server::handlers::auth::digest::AuthCheckerSettings& digest_auth_settings_;
    storages::secdist::Secdist& secdist_;
    storages::postgres::ClusterPtr pg_cluster_;
};

class CheckerProxyFactory final : public server::handlers::auth::AuthCheckerFactoryBase {
public:
    static constexpr std::string_view kAuthType = "digest-proxy";

    explicit CheckerProxyFactory(const components::ComponentContext& context);

    server::handlers::auth::AuthCheckerBasePtr MakeAuthChecker(
        const server::handlers::auth::HandlerAuthConfig& auth_config
    ) const override;

private:
    const server::handlers::auth::digest::AuthCheckerSettings& digest_auth_settings_;
    storages::secdist::Secdist& secdist_;
    storages::postgres::ClusterPtr pg_cluster_;
};

}  // namespace samples::digest_auth
/// [auth checker factory decl]
