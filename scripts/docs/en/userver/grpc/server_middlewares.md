# gRPC server middlewares

@see @ref scripts/docs/en/userver/tutorial/grpc_middleware_service.md

The gRPC server can be extended by middlewares.
Middleware is called on each incoming RPC request.
Different middlewares handle the call in the defined order.
A middleware may decide to reject the call or call the next middleware in the stack.
Middlewares may implement almost any enhancement to the gRPC server including authorization
and authentication, ratelimiting, logging, tracing, audit, etc.

## Default middlewares

There is an @ref ugrpc::server::MiddlewarePipelineComponent component for configuring the middlewares pipeline. 
There are default middlewares:
 - @ref ugrpc::server::middlewares::log::Component
 - @ref ugrpc::server::middlewares::congestion_control::Component
 - @ref ugrpc::server::middlewares::deadline_propagation::Component
 - @ref ugrpc::server::middlewares::baggage::Component
 - @ref ugrpc::server::middlewares::headers_propagator::Component

All of these middlewares are enabled by default. However, **you must register components** of these middlewares in the component list.
You should use @ref ugrpc::server::DefaultComponentList or @ref ugrpc::server::MinimalComponentList.

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

For more info about `enabled` see @ref scripts/docs/en/userver/grpc/middlewares_configuration.md

## Two main classes

There are two main interfaces for implementing a middleware:
1. @ref ugrpc::server::MiddlewareBase. Class that implements the main logic of a middleware
2. @ref ugrpc::server::MiddlewareFactoryComponentBase (the factory for the middleware)
    * Or for simple cases @ref ugrpc::server::SimpleMiddlewareFactoryComponent

## MiddlewareBase

### Handle and Next

There is a method `ugrpc::server::MiddlewareBase::Handle` that is called on each grpc Call (RPC).

@snippet samples/grpc_middleware_service/src/middlewares/server/auth.hpp Middleware declaration

Each middleware must call `ugrpc::server::MiddlewareCallContext::Next` to continue a middleware pipeline.

@snippet samples/grpc_middleware_service/src/middlewares/server/auth.cpp Middleware implementation

Register the component

@snippet samples/grpc_middleware_service/main.cpp server AuthComponent

The static YAML config.

@snippet samples/grpc_middleware_service/static_config.yaml grpc-server-auth static config

### CallRequestHook and CallResponseHook

Also, you can add some behavior on each request/response. Especially, it can be important for grpc-stream. See about streams in @ref scripts/docs/en/userver/grpc/grpc.md

@snippet grpc/functional_tests/middleware_server/src/my_middleware.hpp gRPC CallRequestHook declaration example
@snippet grpc/functional_tests/middleware_server/src/my_middleware.cpp gRPC CallRequestHook impl example

The dtatic YAML config

@snippet grpc/functional_tests/middleware_server/static_config.yaml my-middleware-server config

Register the Middleware component in the component system

@snippet grpc/functional_tests/middleware_server/src/main.cpp register MyMiddlewareComponent

## Middleware order

There are simple cases above: we just set `Auth` group for one middleware and use a default constructor of `MiddlewareDependencyBuilder` in other middleware.
Here we say that all server middlewares are located in these groups.
More info about the middlewares order see @ref scripts/docs/en/userver/grpc/middlewares_order.md

`PreCore` group is called firstly, then `Logging` and so forth...

@dot
digraph Pipeline {
  node [shape=box];
  compound=true;
  fixedsize=true;
  rankdir=LR;
  tooltip = "You didn't hit the arrow with the cursor :-)";
  labeljust = "l";
  labelloc = "t";

  subgraph cluster_User {
    shape=box;
    label = "User";
    center=true;
    rankdir=LR;

    Baggage [label = "grpc-server-baggage", shape=box, width=2.0 ];
    HeadersPropagator [label = "grpc-server-headers-propagator", shape=box, width=2.0 ];


    HeadersPropagator -> Baggage [penwidth=3, dir=both  arrowtail=none];
  }

  subgraph cluster_Core {
    shape=box;
    label = "Core";
    center=true;
    rankdir=LR;

    CongestionControl [label = "congestion-control", shape=box, width=2.0 ];
    DeadlinePropagation [label = "deadline-propagation", shape=box, width=2.0 ];

    DeadlinePropagation -> CongestionControl [penwidth=3, dir=both  arrowtail=none];
  }

  subgraph cluster_Logging {
    shape=box;
    label = "Logging";
    center=true;

    Logging [label = "grpc-server-logging", shape=box, width=2.0 ];
  }

  PreCore [label = "PreCore", shape=box, width=2.0];
  Auth [label = "Auth", shape=box, width=2.0];
  PostCore [label = "PostCore", shape=box, width=2.0];


  Baggage -> PostCore [penwidth=3, minlen=0, dir=both arrowtail = none];
  PostCore -> DeadlinePropagation [penwidth=3, dir=both  arrowtail=none];
  Auth -> Logging [penwidth=3, dir=both  arrowtail=none];
  Logging -> PreCore [penwidth=3, minlen=0, dir=both arrowtail = none];
  CongestionControl -> Auth [penwidth=3, dir=both  arrowtail=none];

  Pipeline[label = "grpc-server-middlewares-pipeline", shape=plaintext, rank="main"];
}
@enddot

## MiddlewareFactoryComponentBase

There are two ways to implement a middleware component:
1. short-cut @ref ugrpc::server::SimpleMiddlewareFactoryComponent
2. @ref ugrpc::server::MiddlewareFactoryComponentBase

Best practice:

If your middleware doesn't use static config options, just use a short-cut.

@warning In that case, `kName` and `kDependency` (@ref middlewares::MiddlewareDependencyBuilder) must be in a Middleware class (as shown above).

If you want to use static config options for your middleware, use @ref ugrpc::server::MiddlewareFactoryComponentBase. See @ref scripts/docs/en/userver/grpc/middlewares_configuration.md

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/grpc/grpc.md | @ref scripts/docs/en/userver/grpc/client_middlewares.md ⇨
@htmlonly </div> @endhtmlonly
