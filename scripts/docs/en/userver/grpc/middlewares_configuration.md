# gRPC middlewares configuration

If your middleware doesn't have static config options, just use:
1. @ref ugrpc::server::SimpleMiddlewareFactoryComponent and see @ref scripts/docs/en/userver/grpc/server_middleware_implementation.md
2. @ref ugrpc::client::SimpleMiddlewareFactoryComponent and see @ref scripts/docs/en/userver/grpc/client_middleware_implementation.md

## Yaml config for a middleware

To use static config options you need inherit from:
1. @ref ugrpc::server::MiddlewareFactoryComponentBase
2. @ref ugrpc::client::MiddlewareFactoryComponentBase

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


@anchor grpc_middlewares_config_override
## Config override

Config options are overrided in your components that are inherited from:

1. @ref ugrpc::server::MiddlewareFactoryComponentBase for the server side.
2. @ref ugrpc::client::MiddlewareFactoryComponentBase for the client side.

You can override a middleware config like overriding an `enabled` option.

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


@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/grpc/middlewares_toggle.md | @ref rabbitmq_driver ⇨
@htmlonly </div> @endhtmlonly

@example samples/grpc_middleware_service/src/middlewares/server/meta_filter.hpp
@example samples/grpc_middleware_service/src/middlewares/server/meta_filter.cpp
@example samples/grpc_middleware_service/main.cpp
