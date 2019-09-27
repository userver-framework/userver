#pragma once

#include <string>

#include <boost/optional.hpp>

#include <server/request/request_context.hpp>
#include <utils/non_loggable.hpp>

#include <server/auth/user_env.hpp>
#include <server/auth/user_id.hpp>
#include <server/auth/user_scopes.hpp>

namespace server::handlers::auth {
class AuthCheckerBase;
}

namespace server::auth {

class UserAuthInfo final {
 public:
  using Ticket = utils::NonLoggable<std::string>;

  explicit UserAuthInfo(UserId default_id);
  UserAuthInfo(UserId default_id, Ticket user_ticket, UserEnv env);

  UserAuthInfo(UserId default_id, UserIds ids, UserScopes scopes, UserEnv env);
  UserAuthInfo(UserId default_id, UserIds ids, UserScopes scopes,
               Ticket user_ticket, UserEnv env);

  UserId GetDefaultUserId() const;
  const UserIds& GetUserIds() const;
  const boost::optional<UserScopes>& GetUserScopesOptional() const;
  const boost::optional<Ticket>& GetTicketOptional() const;
  boost::optional<UserEnv> GetUserEnvOptional() const { return user_env_; }

 private:
  friend class server::handlers::auth::AuthCheckerBase;
  static void Set(server::request::RequestContext& request_context,
                  UserAuthInfo&& info);

  UserId default_id_;
  UserIds ids_;
  boost::optional<UserScopes> scopes_;
  boost::optional<Ticket> user_ticket_;
  boost::optional<UserEnv> user_env_;
};

const UserAuthInfo& GetUserAuthInfo(
    const server::request::RequestContext& request_context);

}  // namespace server::auth
