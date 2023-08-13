#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/ping.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/daemon_run.hpp>

#include "handlers/auth/auth_bearer.hpp"

#include "handlers/profiles/profiles.hpp"
#include "handlers/profiles/profiles_follow.hpp"
#include "handlers/profiles/profiles_follow_delete.hpp"

#include "handlers/tags/tags.hpp"

#include "handlers/comments/comment_delete.hpp"
#include "handlers/comments/comment_post.hpp"
#include "handlers/comments/comments_get.hpp"

#include "handlers/users/user_get.hpp"
#include "handlers/users/user_put.hpp"
#include "handlers/users/users.hpp"
#include "handlers/users/users_login.hpp"

#include "handlers/articles/articles_post.hpp"
#include "handlers/articles/articles_slug_get.hpp"
#include "handlers/articles/articles_slug_put.hpp"
#include "handlers/articles/articles_slug_delete.hpp"
#include "handlers/articles/articles_get.hpp"
#include "handlers/articles/feed_articles.hpp"
#include "handlers/articles/articles_favorite.hpp"
#include "handlers/articles/articles_unfavorite.hpp"

using namespace real_medium::handlers;

int main(int argc, char* argv[]) {
  userver::server::handlers::auth::RegisterAuthCheckerFactory(
      "bearer", std::make_unique<real_medium::auth::CheckerFactory>());

  auto component_list =
      userver::components::MinimalServerComponentList()
          .Append<userver::server::handlers::Ping>()
          .Append<userver::components::TestsuiteSupport>()
          .Append<userver::components::HttpClient>()
          .Append<userver::components::Postgres>("realmedium-database")
          .Append<userver::clients::dns::
                      Component>()  // real_medium::handlers::users_login::post
          .Append<userver::server::handlers::TestsControl>()
          .Append<real_medium::handlers::users::put::Handler>()
          .Append<real_medium::handlers::users::get::Handler>()
          .Append<real_medium::handlers::comments::del::Handler>()
          .Append<real_medium::handlers::comments::post::Handler>()
          .Append<real_medium::handlers::comments::get::Handler>()
          .Append<real_medium::handlers::users::post::RegisterUser>()
          .Append<real_medium::handlers::profiles::get::Handler>()
          .Append<real_medium::handlers::profiles::post::Handler>()
          .Append<real_medium::handlers::profiles::del::Handler>()
          .Append<real_medium::handlers::tags::get::Handler>()
          .Append<real_medium::handlers::articles::feed::get::Handler>()
          .Append<real_medium::handlers::articles::get::Handler>()
          .Append<real_medium::handlers::articles::post::Handler>()
          .Append<real_medium::handlers::articles_slug::get::Handler>()
          .Append<real_medium::handlers::articles_slug::put::Handler>()
          .Append<real_medium::handlers::articles_slug::del::Handler>()
          .Append<real_medium::handlers::articles_favorite::post::Handler>()
          .Append<real_medium::handlers::articles_favorite::del::Handler>()
      ;

  real_medium::handlers::users_login::post::AppendLoginUser(component_list);

  return userver::utils::DaemonMain(argc, argv, component_list);
}
