#include <userver/utest/using_namespace_userver.hpp>

/// [Production service sample - main]
#include <userver/components/common_component_list.hpp>
#include <userver/components/common_server_component_list.hpp>
#include <userver/server/handlers/ping.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/storages/secdist/provider_component.hpp>
#include <userver/utils/daemon_run.hpp>

int main(int argc, char* argv[]) {
  const auto component_list =
      components::ComponentList()
          .AppendComponentList(components::CommonComponentList())
          .AppendComponentList(components::CommonServerComponentList())
          .Append<components::Secdist>()
          .Append<components::DefaultSecdistProvider>()
          .Append<server::handlers::Ping>()

      // Put your handlers and components here
      ;

  return utils::DaemonMain(argc, argv, component_list);
}
/// [Production service sample - main]
