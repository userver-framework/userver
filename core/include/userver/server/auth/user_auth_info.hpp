#pragma once

#include <optional>
#include <string>

#include <userver/server/request/request_context.hpp>
#include <userver/utils/strong_typedef.hpp>

#include <userver/server/auth/user_env.hpp>
#include <userver/server/auth/user_id.hpp>
#include <userver/server/auth/user_provider.hpp>
#include <userver/server/auth/user_scopes.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers::auth {
class AuthCheckerBase;
}

namespace server::auth {

class UserAuthInfo final {
 public:
  using Ticket = utils::NonLoggable<class TicketTag, std::string>;

  UserAuthInfo(UserId default_id, UserEnv env, UserProvider provider);
  UserAuthInfo(UserId default_id, Ticket user_ticket, UserEnv env,
               UserProvider provider);

  UserAuthInfo(UserId default_id, UserIds ids, UserScopes scopes, UserEnv env,
               UserProvider provider);
  UserAuthInfo(UserId default_id, UserIds ids, UserScopes scopes,
               Ticket user_ticket, UserEnv env, UserProvider provider);

  UserId GetDefaultUserId() const;
  const UserIds& GetUserIds() const;
  const std::optional<UserScopes>& GetUserScopesOptional() const;
  const std::optional<Ticket>& GetTicketOptional() const;
  UserEnv GetUserEnv() const { return user_env_; }
  UserProvider GetUserProvider() const { return user_provider_; }

 private:
  friend class server::handlers::auth::AuthCheckerBase;
  static void Set(server::request::RequestContext& request_context,
                  UserAuthInfo&& info);

  UserId default_id_;
  UserIds ids_;
  std::optional<UserScopes> scopes_;
  std::optional<Ticket> user_ticket_;
  UserEnv user_env_;
  UserProvider user_provider_;
};

const UserAuthInfo& GetUserAuthInfo(
    const server::request::RequestContext& request_context);

}  // namespace server::auth

USERVER_NAMESPACE_END
