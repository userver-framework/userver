#include <userver/server/handlers/http_handler_static.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/tracing/manager_component.hpp>
#include <userver/utest/using_namespace_userver.hpp>

#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component_list.hpp>
#include <userver/components/component_list.hpp>
#include <userver/components/fs_cache.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/middlewares/http_middleware_base.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/daemon_run.hpp>

#include <dataset_provider.hpp>
#include <handlers/async_db/handler.hpp>
#include <handlers/baseline11/handler.hpp>
#include <handlers/baseline2/handler.hpp>
#include <handlers/json/handler.hpp>
#include <handlers/plaintext/handler.hpp>
#include <handlers/upload/handler.hpp>
#include <middlewares/compression.hpp>
#include <middlewares/compression_pipeline_builder.hpp>

namespace userver_httparena {
class NoopTracingManager final : public tracing::TracingManagerComponentBase {
public:
    static constexpr std::string_view kName{"noop-tracing-manager"};
    using tracing::TracingManagerComponentBase::TracingManagerComponentBase;

protected:
    bool TryFillSpanBuilderFromRequest(const server::http::HttpRequest&, tracing::SpanBuilder&) const final {
        return true;
    }

    void FillRequestWithTracingContext(const tracing::Span&, clients::http::MiddlewareRequest) const final {}

    void FillResponseWithTracingContext(const tracing::Span&, server::http::HttpResponse&) const final {}
};
}  // namespace userver_httparena

int main(int argc, char* argv[]) {
    auto component_list =
        components::MinimalServerComponentList()
            .Append<clients::dns::Component>()
            .AppendComponentList(clients::http::ComponentList())
            .Append<components::TestsuiteSupport>()
            .Append<server::handlers::TestsControl>()
            .Append<components::FsCache>("fs-cache-static")
            .Append<server::handlers::HttpHandlerStatic>()
            .Append<components::Postgres>("hello-world-db")
            .Append<userver_httparena::plaintext::Handler>()
            .Append<userver_httparena::baseline11::Handler>()
            .Append<userver_httparena::json::Handler>()
            .Append<userver_httparena::upload::Handler>()
            .Append<userver_httparena::async_db::Handler>()
            .Append<userver_httparena::baseline2::Handler>()
            .Append<userver_httparena::DatasetProvider>()
            .Append<server::middlewares::SimpleHttpMiddlewareFactory<
                userver_httparena::middlewares::CompressionMiddleware>>()
            .Append<userver_httparena::middlewares::CompressionPipelineBuilder>()
            .Append<userver_httparena::NoopTracingManager>();
    return utils::DaemonMain(argc, argv, component_list);
}
