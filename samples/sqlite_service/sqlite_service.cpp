#include <userver/clients/dns/component.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utest/using_namespace_userver.hpp>

#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/ping.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/daemon_run.hpp>

#include <userver/storages/sqlite.hpp>

namespace samples::sqlite {

namespace {

std::string SayHelloTo(std::string_view name) {
    if (name.empty()) {
        name = "unknown user";
    }

    return fmt::format("Hello, {}!\n", name);
}

}  // namespace

class Hello final : public server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-hello";

    using HttpHandlerBase::HttpHandlerBase;

    std::string HandleRequestThrow(const server::http::HttpRequest& request, server::request::RequestContext&)
        const override {
        return SayHelloTo(request.GetArg("name"));
    }
};

int main(int argc, char* argv[]) {
    auto component_list = components::MinimalServerComponentList()
                              .Append<Hello>()
                              .Append<server::handlers::Ping>()
                              .Append<components::TestsuiteSupport>()
                              .Append<components::HttpClient>()
                              .Append<clients::dns::Component>()
                              .Append<server::handlers::TestsControl>();

    return utils::DaemonMain(argc, argv, component_list);
}

}  // namespace samples::sqlite

int main(int argc, char* argv[]) { return samples::sqlite::main(argc, argv); }
