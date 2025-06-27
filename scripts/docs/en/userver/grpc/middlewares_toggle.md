# Enable/disable middlewares

## Enable middlewares per service or per client

### The simplest case

You just enable middleware globally by default.

```yaml
components_manager:
    components:
        grpc-server-middlewares-pipeline:
            middlewares:
                some-middleware:
                    enabled: true  # globally enable
```

### Particular enable/disable for service or client

There are some ways to enable/disable for a particular service/client.

1\. Globally enable (for all services) and local disable

For example, there is a auth middleware, but some service don't require the auth middleware by some reasons.

```yaml
components_manager:
    components:
        grpc-server-middlewares-pipeline:
            middlewares:
                some-middleware:
                    enabled: true  # globally enable
        some-service:
            middlewares:
                some-middleware:
                    enabled: false  # locally disable
```

2\. Globally disable (for all services) and force local enable

For example, you want to start test some middleware in a particular service or client.

```yaml
components_manager:
    components:
        grpc-server-middlewares-pipeline:
            middlewares:
                some-middleware:
                    enabled: false  # globally disable
        some-service:
            middlewares:
                some-middleware:
                    enabled: true  # force enable in that service
```

For instructions on how to enable/disable middlewares per-service or per-client:
@see @ref grpc_middlewares_disable_all


@anchor grpc_middlewares_disable_all
## How to disable all middlewares

There are two additional options in the static config:
1. `disable-all-pipeline-middlewares`
2. `disable-user-pipeline-middlewares`

`disable-all-pipeline-middlewares` may be useful for "system" handlers and clients. E.g. `grpc-health`.

`disable-user-pipeline-middlewares` may be useful for libraries that don't want to receive unknown `User` middlewares to their library middlewares list.

@warning It is dangerous to disable all middlewares! Some middlewares greatly enhance service resilency and availability (e.g., @ref ugrpc::server::middlewares::deadline_propagation::Component, @ref ugrpc::server::middlewares::congestion_control::Component, @ref ugrpc::client::middlewares::deadline_propagation::Component).

For more information about options see docs `ugrpc::server::ServiceComponentBase` and `ugrpc::client::ClientFactoryComponent`. 

Example of the static YAML config.

```yaml
        grpc-client-factory:
            channel-args: {}
            # Disable all middlewares. But you can force enable some middlewares.
            disable-all-pipeline-middlewares: true
            middlewares:
                grpc-client-logging:  # force enable
                    enabled: true

```


@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/grpc/middlewares_order.md | @ref scripts/docs/en/userver/grpc/middlewares_configuration.md ⇨
@htmlonly </div> @endhtmlonly
