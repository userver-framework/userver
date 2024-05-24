#include <userver/utest/using_namespace_userver.hpp>

/// [Hello service sample - main]
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/utils/daemon_run.hpp>

#include "hello_service.hpp"

int main(int argc, char* argv[]) {
  auto component_list = components::MinimalServerComponentList();
  samples::hello::AppendHello(component_list);

  return utils::DaemonMain(argc, argv, component_list);
}
/// [Hello service sample - main]
