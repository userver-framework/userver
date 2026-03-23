#include <userver/clients/dns/component.hpp>
#include <userver/components/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/storages/scylla/component.hpp>
#include <userver/storages/scylla/operations.hpp>
#include <userver/storages/scylla/session.hpp>
#include <userver/utest/using_namespace_userver.hpp>
#include <userver/utils/daemon_run.hpp>

/// [Scylla service sample - component]
namespace samples::scylladb {

class Example final : public server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-example";

    Example(const components::ComponentConfig& config, const components::ComponentContext& context)
        : HttpHandlerBase(config, context),
          session_(context.FindComponent<components::Scylla>("scylla-example").GetSession()) {}

    std::string HandleRequest(server::http::HttpRequest&, server::request::RequestContext&) const override {
        InsertNew();
        return "OK";
    }

private:
    void InsertNew() const;

    storages::scylla::SessionPtr session_;
};

}  // namespace samples::scylladb
/// [Scylla service sample - component]

namespace samples::scylladb {

/// [Scylla service sample - InsertNew]
void Example::InsertNew() const {
    auto table = session_->GetTable("basic");

    storages::scylla::operations::InsertOne op;
    op.BindString("key", "test");
    op.BindBool("bln", true);
    op.BindFloat("flt", 0.001f);
    op.BindDouble("dbl", 0.0002);
    op.BindInt32("i32", 1);
    op.BindInt64("i64", 2);

    table.Execute(op);
}
/// [Scylla service sample - InsertNew]

}  // namespace samples::scylladb

/// [Scylla service sample - main]
int main(int argc, char* argv[]) {
    const auto component_list =
        components::MinimalServerComponentList()
            .Append<clients::dns::Component>()
            .Append<components::Scylla>("scylla-example")
            .Append<samples::scylladb::Example>();
    return utils::DaemonMain(argc, argv, component_list);
}
/// [Scylla service sample - main]