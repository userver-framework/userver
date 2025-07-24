# gRPC client middlewares

@see @ref scripts/docs/en/userver/tutorial/grpc_middleware_service.md.

The gRPC client can be extended by middlewares.
Middleware is called on each outgoing RPC request and incoming response.
Different middlewares handle the call in the defined order.
A middleware may decide to reject the call or call the next middleware in the stack.
Middlewares may implement almost any enhancement to the gRPC client including authorization
and authentication, ratelimiting, logging, tracing, audit, etc.

## Default middlewares

There is a `ugrpc::client::MiddlewarePipelineComponent` component for configuring the middlewares's pipeline. 
There are default middlewares:
 - @ref ugrpc::client::middlewares::log::Component
 - @ref ugrpc::client::middlewares::deadline_propagation::Component
 - @ref ugrpc::client::middlewares::baggage::Component
 - @ref ugrpc::client::middlewares::headers_propagator::Component
 - @ref ugrpc::client::middlewares::testsuite::Component (used only in testsuite)

If you add these middlewares to the @ref components::ComponentList, these middlewares will be enabled by default. 
To register core gRPC client components and a set of builtin middlewares use @ref ugrpc::client::DefaultComponentList or @ref ugrpc::client::MinimalComponentList.
As will be shown below, custom middlewares require additional actions to work: registering in `grpc-client-middleware-pipeline` and writing a required static config entry.

`ugrpc::client::MiddlewarePipelineComponent` is a global configuration of client middlewares.
If you don't want to disable userver middlewares, just take that config:

```yaml
components_manager:
    components:
        grpc-client-middlewares-pipeline:
        grpc-client-common:
            # some options...
        some-client-factory:
            # some client options...
```

### Enable/disable middlewares

You can enable or disable any middleware:

```yaml
components_manager:
    components:
        grpc-client-common:

        grpc-client-middlewares-pipeline:
            middlewares:
                grpc-client-baggage:
                    enabled: false  # globally disable for all clients

        some-client-factory:
            middlewares:
                # force enable in that client. Or it can be disabled for special clients
                grpc-client-baggage:
                    enabled: true

```

For more information about `enabled`:
@see @ref scripts/docs/en/userver/grpc/middlewares_toggle.md.


## Static config options of a middleware

How to override a static config of a middleware: @ref grpc_middlewares_config_override.

How to implement your own middleware with a static config: @ref scripts/docs/en/userver/grpc/middlewares_configuration.md.


## How to implement your own middleware

@ref scripts/docs/en/userver/grpc/client_middleware_implementation.md.


@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/grpc/server_middleware_implementation.md | @ref scripts/docs/en/userver/grpc/client_middleware_implementation.md ⇨
@htmlonly </div> @endhtmlonly
