#include <userver/utest/using_namespace_userver.hpp>

#include <fmt/format.h>

#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/components/component_list.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/etcd/component.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/handlers/ping.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/daemon_run.hpp>

namespace {

class HandlerV1Get final : public server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-v1-get";

    HandlerV1Get(const components::ComponentConfig& config, const components::ComponentContext& component_context)
        : HttpHandlerBase(config, component_context),
          etcd_client_ptr_(component_context.FindComponent<etcd::Component>("etcd-client").GetClient()) {}

    std::string HandleRequestThrow(const server::http::HttpRequest& request, server::request::RequestContext&)
        const override {
        const auto maybe_value = etcd_client_ptr_->Get(request.GetArg("key"));
        return maybe_value.value_or("No value");
    }

private:
    etcd::ClientPtr etcd_client_ptr_;
};

class HandlerV1Put final : public server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-v1-put";

    HandlerV1Put(const components::ComponentConfig& config, const components::ComponentContext& component_context)
        : HttpHandlerBase(config, component_context),
          etcd_client_ptr_(component_context.FindComponent<etcd::Component>("etcd-client").GetClient()) {}

    std::string HandleRequestThrow(const server::http::HttpRequest& request, server::request::RequestContext&)
        const override {
        etcd_client_ptr_->Put(request.GetArg("key"), request.GetArg("value"));

        return std::string();
    }

private:
    etcd::ClientPtr etcd_client_ptr_;
};

class HandlerV1Watch final : public server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-v1-watch";

    HandlerV1Watch(const components::ComponentConfig& config, const components::ComponentContext& component_context)
        : HttpHandlerBase(config, component_context),
          etcd_client_ptr_(component_context.FindComponent<etcd::Component>("etcd-client").GetClient()) {}

    std::string HandleRequestThrow(const server::http::HttpRequest& request, server::request::RequestContext&)
        const override {
        const auto key = request.GetArg("key");
        const auto maybe_original_value = etcd_client_ptr_->Get(key);
        auto watch_listener = etcd_client_ptr_->StartWatch(key);
        const auto watch_event = watch_listener.GetEvent();
        const auto new_value = watch_event.value;
        return fmt::format("original value: {}, new value: {}", maybe_original_value.value_or("No value"), new_value);
    }

private:
    etcd::ClientPtr etcd_client_ptr_;
};

}  // namespace

int main(int argc, char* argv[]) {
    auto component_list = components::MinimalServerComponentList()
                              .Append<server::handlers::Ping>()
                              .Append<components::TestsuiteSupport>()
                              .Append<components::HttpClient>()
                              .Append<clients::dns::Component>()
                              .Append<etcd::Component>()
                              .Append<server::handlers::TestsControl>()
                              .Append<HandlerV1Get>()
                              .Append<HandlerV1Put>()
                              .Append<HandlerV1Watch>();

    return utils::DaemonMain(argc, argv, component_list);
}
