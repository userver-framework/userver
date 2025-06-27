# gRPC server middlewares

@see @ref scripts/docs/en/userver/tutorial/grpc_middleware_service.md.

The gRPC server can be extended by middlewares.
Middleware hooks are called at the various corresponding stages of handling of each incoming RPC.
Different middlewares handle the call in the defined order.
A middleware may decide to reject the call or call the next middleware in the stack.
Middlewares may implement almost any enhancement to the gRPC server including authorization
and authentication, ratelimiting, logging, tracing, audit, etc.

## Default middlewares

There is an @ref ugrpc::server::MiddlewarePipelineComponent component for configuring the middlewares pipeline. 
There are default middlewares:
 - @ref ugrpc::server::middlewares::access_log::Component (not included in default lists)
 - @ref ugrpc::server::middlewares::log::Component
 - @ref ugrpc::server::middlewares::congestion_control::Component
 - @ref ugrpc::server::middlewares::deadline_propagation::Component
 - @ref ugrpc::server::middlewares::baggage::Component
 - @ref ugrpc::server::middlewares::headers_propagator::Component

If you add these middlewares to the @ref components::ComponentList, these middlewares will be enabled by default. 
To register core gRPC server components and a set of builtin middlewares use @ref ugrpc::server::DefaultComponentList or @ref ugrpc::server::MinimalComponentList.
As will be shown below, custom middlewares require additional actions to work: registering in `grpc-server-middleware-pipeline` and writing a required static config entry.

@ref ugrpc::server::MiddlewarePipelineComponent is a component for a global configuration of server middlewares. You can enable/disable middlewares with `enabled` option.

If you don't want to disable userver middlewares, just take that config:

```yaml
components_manager:
    components:
        grpc-server-middlewares-pipeline:
        grpc-server:
            # some server options...
        some-service:
            # some service options...
```

### Enable/disable middlewares

You can enable or disable any middleware:

```yaml
components_manager:
    components:
        grpc-server:

        grpc-server-middlewares-pipeline:
            middlewares:
                grpc-server-headers-propagator:
                    enabled: false  # globally disable for all services

        some-service:
            middlewares:
                # force enable in that service. Or it can be disabled for special service
                grpc-server-headers-propagator:
                    enabled: true

```

For more information about `enabled`:
@see @ref scripts/docs/en/userver/grpc/middlewares_toggle.md.


## Static config options of a middleware

How to override a static config of a middleware: @ref grpc_middlewares_config_override.

How to implement your own middleware with a static config: @ref scripts/docs/en/userver/grpc/middlewares_configuration.md.


## How to implement your own middleware

@ref scripts/docs/en/userver/grpc/server_middleware_implementation.md.


@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/grpc/grpc.md | @ref scripts/docs/en/userver/grpc/server_middleware_implementation.md ⇨
@htmlonly </div> @endhtmlonly
