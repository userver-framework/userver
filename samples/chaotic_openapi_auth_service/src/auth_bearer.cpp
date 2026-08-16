#include "auth_bearer.hpp"

#include <cstdint>
#include <string>

#include <userver/http/common_headers.hpp>
#include <userver/server/auth/user_auth_info.hpp>
#include <userver/server/handlers/exceptions.hpp>

namespace samples::auth {

class AuthCheckerBearer final : public server::handlers::auth::AuthCheckerBase {
public:
    using AuthCheckResult = server::handlers::auth::AuthCheckResult;

    [[nodiscard]] AuthCheckResult CheckAuth(
        const server::http::HttpRequest& request,
        server::request::RequestContext& request_context
    ) const override;

    [[nodiscard]] bool SupportsUserAuth() const noexcept override { return true; }
};

/// [auth checker definition]
AuthCheckerBearer::AuthCheckResult AuthCheckerBearer::CheckAuth(
    const server::http::HttpRequest& request,
    server::request::RequestContext& request_context
) const {
    const auto& auth_value = request.GetHeader(http::headers::kAuthorization);
    constexpr std::string_view kBearerPrefix = "Bearer ";
    if (auth_value.size() <= kBearerPrefix.size() ||
        std::string_view{auth_value}.substr(0, kBearerPrefix.size()) != kBearerPrefix) {
        return AuthCheckResult{
            AuthCheckResult::Status::kTokenNotFound,
            {},
            "Bearer token is required",
            server::handlers::HandlerErrorCode::kUnauthorized,
        };
    }

    std::uint64_t user_id = 0;
    try {
        user_id = std::stoull(std::string{auth_value.substr(kBearerPrefix.size())});
    } catch (const std::exception&) {
        return AuthCheckResult{
            AuthCheckResult::Status::kInvalidToken,
            {},
            "User id in the token must be an integer",
            server::handlers::HandlerErrorCode::kUnauthorized,
        };
    }

    SetUserAuthInfo(
        request_context,
        server::auth::UserAuthInfo{
            server::auth::UserId{user_id},
            server::auth::UserEnv::kProd,
            server::auth::UserProvider::kYandex,
        }
    );
    return {};
}
/// [auth checker definition]

CheckerFactory::CheckerFactory(const components::ComponentContext&) {}

server::handlers::auth::AuthCheckerBasePtr CheckerFactory::MakeAuthChecker(
    const server::handlers::auth::HandlerAuthConfig&
) const {
    return std::make_shared<AuthCheckerBearer>();
}

}  // namespace samples::auth