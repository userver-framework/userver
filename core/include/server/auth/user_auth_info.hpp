#pragma once

#include <string>

#include <boost/optional.hpp>

#include <formats/parse/to.hpp>
#include <server/request/request_context.hpp>

#include <server/auth/user_id.hpp>
#include <server/auth/user_scopes.hpp>

namespace server::handlers::auth {
class AuthCheckerBase;
}

namespace server::auth {

class UserAuthInfo {
 public:
  explicit UserAuthInfo(UserId default_id);
  UserAuthInfo(UserId default_id, std::string user_ticket);

  UserAuthInfo(UserId default_id, UserIds ids, UserScopes scopes);
  UserAuthInfo(UserId default_id, UserIds ids, UserScopes scopes,
               std::string user_ticket);

  UserId GetDefaultUserId() const;
  const UserIds& GetUserIds() const;
  const boost::optional<UserScopes>& GetUserScopesOptional() const;
  const boost::optional<std::string>& GetTicketOptional() const;

 private:
  friend class server::handlers::auth::AuthCheckerBase;
  static void Set(server::request::RequestContext& request_context,
                  UserAuthInfo&& info);

  UserId default_id_;
  UserIds ids_;
  boost::optional<UserScopes> scopes_;
  boost::optional<std::string> user_ticket_;
};

const UserAuthInfo& GetUserAuthInfo(
    const server::request::RequestContext& request_context);

}  // namespace server::auth
