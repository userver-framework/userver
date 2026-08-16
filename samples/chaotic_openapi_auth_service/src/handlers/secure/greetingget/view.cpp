#include "view.hpp"

#include <fmt/format.h>

#include <handlers/secure/greetingget/handler.hpp>
#include <userver/server/auth/user_auth_info.hpp>

namespace handlers::secure::greetingget {

/// [view-impl-auth]
Response View::Handle(
    Request&& /*request*/,
    Deps&& /*deps*/,
    RequestContext& context
) {
    const auto& auth_info = USERVER_NAMESPACE::server::auth::GetUserAuthInfo(context);
    const auto user_id = auth_info.GetDefaultUserId();
    return Response200{
        .body = {.greeting = fmt::format("Hello, user {}!", USERVER_NAMESPACE::server::auth::ToUInt64(user_id))}
    };
}
/// [view-impl-auth]

}  // namespace handlers::secure::greetingget