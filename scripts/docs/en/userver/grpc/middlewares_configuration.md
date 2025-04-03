# gRPC middlewares configuration

If your middleware doesn't have static config options, just use @ref ugrpc::server::SimpleMiddlewareFactoryComponent (identical for a client).

## Yaml config for a middleware

To use static config options you need inherit from @ref ugrpc::server::MiddlewareFactoryComponentBase. Example for a server (identical for a client)

@snippet samples/grpc_middleware_service/src/middlewares/server/meta_filter.hpp gRPC middleware sample
@snippet samples/grpc_middleware_service/src/middlewares/server/meta_filter.cpp gRPC middleware sample

Out middleware will be in the @ref middlewares::groups::User and will be called after headers_propagator middleware
@note More info about middlewares order @ref scripts/docs/en/userver/grpc/middlewares_order.md

Then register these components in `components::ComponentList`

@snippet samples/grpc_middleware_service/main.cpp server MetaFilterComponent

The static yaml config of middleware. You must register it in the config `grpc-server-middlewares-pipeline`

```yaml
        grpc-server-meta-filter:
            headers:
              - global-header
 
        grpc-server-middlewares-pipeline:
            middlewares:
                grpc-server-meta-filter:  # register the middleware in the pipeline
                    enabled: true
```

@snippet samples/grpc_middleware_service/static_config.yaml gRPC middleware sample - static config server middleware

To see full src:

@example samples/grpc_middleware_service/src/middlewares/server/meta_filter.hpp
@example samples/grpc_middleware_service/src/middlewares/server/meta_filter.cpp
@example samples/grpc_middleware_service/main.cpp

## Config override

To a middleware config per service or client (like overriding `enabled` option).

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
⇦ @ref scripts/docs/en/userver/grpc/middlewares_order.md | @ref rabbitmq_driver ⇨
@htmlonly </div> @endhtmlonly
