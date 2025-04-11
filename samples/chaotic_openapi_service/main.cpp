#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/utest/using_namespace_userver.hpp>
#include <userver/utils/daemon_run.hpp>

// Note: this is for the purposes of tests/samples only
#include <userver/utest/using_namespace_userver.hpp>

#include <client/test/component.hpp>

#include <hello_handler.hpp>

int main(int argc, char* argv[]) {
    auto component_list = components::MinimalServerComponentList()
                              .Append<samples::hello::HelloHandler>()
                              .Append<components::HttpClient>()
                              .Append<USERVER_NAMESPACE::clients::dns::Component>()
                              .Append<::clients::test::Component>();

    return utils::DaemonMain(argc, argv, component_list);
}
