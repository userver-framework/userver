#include <userver/clients/dns/component.hpp>
#include <userver/components/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/storages/scylla/component.hpp>
#include <userver/storages/scylla/operations.hpp>
#include <userver/storages/scylla/session.hpp>
#include <userver/utest/using_namespace_userver.hpp>
#include <userver/utils/daemon_run.hpp>

/// [Scylla service sample - components]
namespace samples::scylladb {

class InsertHandler final : public server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-insert";

    InsertHandler(const components::ComponentConfig& config, const components::ComponentContext& context)
        : HttpHandlerBase(config, context),
          session_(context.FindComponent<components::Scylla>("scylla-example").GetSession()) {}

    std::string HandleRequest(server::http::HttpRequest&, server::request::RequestContext&) const override {
        auto table = session_->GetTable("basic");

        storages::scylla::operations::InsertOne op;
        op.BindString("key", "test");
        op.BindBool("bln", true);
        op.BindFloat("flt", 0.001f);
        op.BindDouble("dbl", 0.0002);
        op.BindInt32("i32", 1);
        op.BindInt64("i64", 2);

        table.Execute(op);
        return "OK";
    }

private:
    storages::scylla::SessionPtr session_;
};

class SelectHandler final : public server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-select";

    SelectHandler(const components::ComponentConfig& config, const components::ComponentContext& context)
        : HttpHandlerBase(config, context),
          session_(context.FindComponent<components::Scylla>("scylla-example").GetSession()) {}

    std::string HandleRequest(server::http::HttpRequest&, server::request::RequestContext&) const override {
        auto table = session_->GetTable("basic");

        storages::scylla::operations::SelectOne op;
        op.AddAllColumns();
        op.WhereString("key", "test");

        auto row = table.Execute(op);

        std::string result;
        for (const auto& [name, value] : row) {
            result += name + "=";
            std::visit([&](const auto& v) {
                using T = std::decay_t<decltype(v)>;
                if constexpr (std::is_same_v<T, std::string>) {
                    result += v;
                } else {
                    result += std::to_string(v);
                }
            }, value);
            result += "\n";
        }
        return result;
    }

private:
    storages::scylla::SessionPtr session_;
};

}  // namespace samples::scylladb
/// [Scylla service sample - components]

/// [Scylla service sample - main]
int main(int argc, char* argv[]) {
    const auto component_list =
        components::MinimalServerComponentList()
            .Append<clients::dns::Component>()
            .Append<components::Scylla>("scylla-example")
            .Append<samples::scylladb::InsertHandler>()
            .Append<samples::scylladb::SelectHandler>();
    return utils::DaemonMain(argc, argv, component_list);
}
/// [Scylla service sample - main]
