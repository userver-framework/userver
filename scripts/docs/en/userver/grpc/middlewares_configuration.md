# gRPC middlewares configuration

If your middleware doesn't have static config options, just use:
1. @ref ugrpc::server::SimpleMiddlewareFactoryComponent 
2. @ref ugrpc::client::SimpleMiddlewareFactoryComponent

## Yaml config for a middleware

To use static config options you need inherit from:
1. @ref ugrpc::server::MiddlewareFactoryComponentBase
1. @ref ugrpc::client::MiddlewareFactoryComponentBase

@snippet samples/grpc_middleware_service/src/middlewares/server/meta_filter.hpp gRPC middleware sample
@snippet samples/grpc_middleware_service/src/middlewares/server/meta_filter.cpp gRPC middleware sample

Our middleware will be in the @ref middlewares::groups::User and will be called after `headers_propagator` middleware.
For more information about middlewares order:
@see @ref scripts/docs/en/userver/grpc/middlewares_order.md.

Then register these components in @ref components::ComponentList. See `sample::grpc::auth::server::MetaFilterComponent`

@snippet samples/grpc_middleware_service/main.cpp gRPC middleware sample - components registration

The static yaml config of middleware. You must add middleware to the config `grpc-server-middlewares-pipeline`, otherwise the middleware will be disabled everywhere.

```yaml
        grpc-server-meta-filter:
            headers:
              - global-header
 
        grpc-server-middlewares-pipeline:
            middlewares:
                grpc-server-meta-filter:  # register the middleware in the pipeline
                    enabled: true
```

See full srcs:

* @ref samples/grpc_middleware_service/src/middlewares/server/meta_filter.hpp	
* @ref samples/grpc_middleware_service/src/middlewares/server/meta_filter.cpp	
* @ref samples/grpc_middleware_service/main.cpp


@anchor grpc_server_middlewares_config_override
## Config override

You can override a middleware config per service or client (like overriding `enabled` option).

Example.

```yaml
        grpc-server-meta-filter:
            headers:
              - global-header
        grpc-server-middlewares-pipeline:
            middlewares:
                grpc-server-meta-filter:  # register the middleware in the pipeline
                    enabled: true
        some-service:
            middlewares:
                grpc-server-meta-filter:
                    # The middleware of this service will get this header instead of 'global-header'
                    headers:
                      - specific-header
```

@note Imagine there are more options but you override only `headers`. Values of other options will be taken from a global config (from `grpc-server-middlewares-pipeline`)


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

1. Globally enable (for all services) and local disable

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
                    enabled: false  # loacally disable
```

2. Globally disable (for all services) and force local enable

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


@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/grpc/middlewares_order.md | @ref rabbitmq_driver ⇨
@htmlonly </div> @endhtmlonly

@example samples/grpc_middleware_service/src/middlewares/server/meta_filter.hpp
@example samples/grpc_middleware_service/src/middlewares/server/meta_filter.cpp
@example samples/grpc_middleware_service/main.cpp
