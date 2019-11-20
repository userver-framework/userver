#pragma once

#include <string>

#include <boost/optional.hpp>

#include <server/request/request_context.hpp>
#include <utils/non_loggable.hpp>

#include <server/auth/user_env.hpp>
#include <server/auth/user_id.hpp>
#include <server/auth/user_provider.hpp>
#include <server/auth/user_scopes.hpp>

namespace server::handlers::auth {
class AuthCheckerBase;
}

namespace server::auth {

class UserAuthInfo final {
 public:
  using Ticket = utils::NonLoggable<std::string>;

  UserAuthInfo(UserId default_id, UserEnv env, UserProvider provider);
  UserAuthInfo(UserId default_id, Ticket user_ticket, UserEnv env,
               UserProvider provider);

  UserAuthInfo(UserId default_id, UserIds ids, UserScopes scopes, UserEnv env,
               UserProvider provider);
  UserAuthInfo(UserId default_id, UserIds ids, UserScopes scopes,
               Ticket user_ticket, UserEnv env, UserProvider provider);

  UserId GetDefaultUserId() const;
  const UserIds& GetUserIds() const;
  const boost::optional<UserScopes>& GetUserScopesOptional() const;
  const boost::optional<Ticket>& GetTicketOptional() const;
  UserEnv GetUserEnv() const { return user_env_; }
  UserProvider GetUserProvider() const { return user_provider_; }

 private:
  friend class server::handlers::auth::AuthCheckerBase;
  static void Set(server::request::RequestContext& request_context,
                  UserAuthInfo&& info);

  UserId default_id_;
  UserIds ids_;
  boost::optional<UserScopes> scopes_;
  boost::optional<Ticket> user_ticket_;
  UserEnv user_env_;
  UserProvider user_provider_;
};

const UserAuthInfo& GetUserAuthInfo(
    const server::request::RequestContext& request_context);

}  // namespace server::auth
