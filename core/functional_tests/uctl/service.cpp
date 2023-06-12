#include <userver/utest/using_namespace_userver.hpp>

#include <userver/cache/caching_component_base.hpp>
#include <userver/cache/lru_cache_component_base.hpp>
#include <userver/components/common_component_list.hpp>
#include <userver/components/common_server_component_list.hpp>
#include <userver/dump/common.hpp>
#include <userver/dump/common_containers.hpp>
#include <userver/server/handlers/ping.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/storages/secdist/provider_component.hpp>
#include <userver/tracing/manager_component.hpp>
#include <userver/utils/daemon_run.hpp>

int main(int argc, const char* const argv[]) {
  const auto component_list =
      components::ComponentList()
          .AppendComponentList(components::CommonComponentList())
          .AppendComponentList(components::CommonServerComponentList())
          .Append<components::Secdist>()
          .Append<components::DefaultSecdistProvider>()
          .Append<server::handlers::Ping>();
  return utils::DaemonMain(argc, argv, component_list);
}
