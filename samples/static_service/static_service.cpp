#include <userver/utest/using_namespace_userver.hpp>

#include <userver/components/fs_cache.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/http_handler_static.hpp>
#include <userver/utils/daemon_run.hpp>

/// [Static service sample - main]
int main(int argc, char* argv[]) {
  const auto component_list =
      components::MinimalServerComponentList()
          .Append<components::FsCache>("fs-cache-main")
          .Append<server::handlers::HttpHandlerStatic>();
  return utils::DaemonMain(argc, argv, component_list);
}
/// [Static service sample - main]
