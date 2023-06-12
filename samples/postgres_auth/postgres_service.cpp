#include "auth_bearer.hpp"
#include "user_info_cache.hpp"

#include <userver/utest/using_namespace_userver.hpp>

#include <userver/clients/dns/component.hpp>
#include <userver/testsuite/testsuite_support.hpp>

#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/utils/daemon_run.hpp>

#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>

namespace samples::pg {

/// [request context]
class Hello final : public server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-hello";

  using HttpHandlerBase::HttpHandlerBase;

  std::string HandleRequestThrow(
      const server::http::HttpRequest&,
      server::request::RequestContext& ctx) const override {
    return "Hello world, " + ctx.GetData<std::string>("name") + "!\n";
  }
};
/// [request context]

}  // namespace samples::pg

/// [auth checker registration]
int main(int argc, const char* const argv[]) {
  server::handlers::auth::RegisterAuthCheckerFactory(
      "bearer", std::make_unique<samples::pg::CheckerFactory>());
  /// [auth checker registration]

  /// [main]
  const auto component_list = components::MinimalServerComponentList()
                                  .Append<samples::pg::AuthCache>()
                                  .Append<components::Postgres>("auth-database")
                                  .Append<samples::pg::Hello>()
                                  .Append<components::TestsuiteSupport>()
                                  .Append<clients::dns::Component>();
  return utils::DaemonMain(argc, argv, component_list);
  /// [main]
}
/// [Postgres service sample - main]
