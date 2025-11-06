#include <userver/etcd/component.hpp>

#include <etcd/client_impl.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/etcd/settings.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

USERVER_NAMESPACE_BEGIN

namespace etcd {

Component::Component(const components::ComponentConfig& config, const components::ComponentContext& context)
    : ComponentBase(config, context),
      etcd_client_ptr_(std::make_shared<impl::ClientImpl>(
          context.FindComponent<components::HttpClient>().GetHttpClient(),
          config.As<ClientSettings>()
      )) {}

yaml_config::Schema Component::GetStaticConfigSchema() {
    return yaml_config::MergeSchemas<ComponentBase>(R"(
type: object
description: Etcd cluster component
additionalProperties: false
properties:
    endpoints:
        type: array
        description: Etcd endpoints to which client make HTTP requests
        items:
            type: string
            description: Etcd endpoint, e.g. http://localhost:2379
    attempts:
        type: integer
        description: >
            Number of attempts to each endpoint, on failed attempts client randomly moves to another endpoint
        minimum: 1
    request_timeout_ms:
        type: integer
        description: Timeout for all HTTP requests to etcd except watch request
        minimum: 1
    watch_timeout_ms:
        type: integer
        description: >
            Timeout for watch HTTP request. It's a stremed request, so it is used also as a connection timeout, so it should not be too short
        minimum: 1
)");
}

ClientPtr Component::GetClient() { return etcd_client_ptr_; }

}  // namespace etcd

USERVER_NAMESPACE_END
