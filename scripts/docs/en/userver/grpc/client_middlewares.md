# gRPC client middlewares

@see @ref scripts/docs/en/userver/tutorial/grpc_middleware_service.md

The gRPC client can be extended by middlewares.
Middleware is called on each outgoing (for client) RPC request.
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
 - @ref ugrpc::client::middlewares::testsuite::Component (used only in testsuite)

All of these middlewares are enabled by default. However, you must register components of these middlewares in the component list.
You should use `ugrpc::client::DefaultComponentList` or `ugrpc::client::MinimalComponentList`.

`ugrpc::client::MiddlewarePipelineComponent` is a global configuration of client middlewares. So, you can enable/disable middlewares with the option `enabled` in the global (`grpc-client-middlewares-pipeline`) or middleware config.

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

For more info about `enabled` see @ref scripts/docs/en/userver/grpc/middlewares_configuration.md

## Two main classes

There are two main interfaces for implementing a middleware:
1. `ugrpc::client::MiddlewareBase`. Class that implements the main logic of a middleware
2. `ugrpc::client::MiddlewareFactoryComponentBase` (the factory for the middleware)
    * Or for simple cases @ref ugrpc::client::SimpleMiddlewareFactoryComponent

## MiddlewareBase

@ref ugrpc::client::MiddlewareBase

### PreStartCall and PostFinish

`PreStartCall` is called before the first message is sent.
`PostFinish` is called after the last message is received.

These hooks are called on each call (even for the streeaming rpc)

@snippet samples/grpc_middleware_service/src/middlewares/client/auth.hpp Middleware declaration
@snippet samples/grpc_middleware_service/src/middlewares/client/auth.cpp gRPC middleware sample - Middleware implementation

Register the Middleware component in the component system

@snippet samples/grpc_middleware_service/main.cpp client AuthComponent

The static YAML config.

@snippet samples/grpc_middleware_service/static_config.yaml static config grpc-auth-client

### PreSendMessage and PostRecvMessage

`PreSendMessage`:
    * unary: is called exactly once
    * stream: is called 0, 1 or more
`PostRecvMessage`
    * unary: is called 0 or 1 (0 if service doesn't return a message)
    * stream: is called 0, 1 or more

@snippet grpc/src/ugrpc/client/middlewares/log/middleware.hpp MiddlewareBase example declaration
@snippet grpc/src/ugrpc/client/middlewares/log/middleware.cpp MiddlewareBase Message methods example

The static YAML config and component registration are edentical as in example above. So, let's not focus on this.

## Middlewares order

There are simple cases above: we just set `Auth` group for one middleware and use a default constructor of `MiddlewareDependencyBuilder` in other middleware.

Here we say that all client middlewares are located in these groups.
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

    Baggage [label = "grpc-client-baggage", shape=box, width=2.0 ];
    Testsuite [label = "grpc-client-testsuite", shape=box, width=2.0 ];

    Testsuite -> Baggage [penwidth=3, dir=both  arrowtail=none];
  }

  subgraph cluster_Core {
    shape=box;
    label = "Core";
    center=true;
    rankdir=LR;

    DeadlinePropagation [label = "grpc-client-deadline-propagation", shape=box, width=2.0 ];
  }

  subgraph cluster_Logging {
    shape=box;
    label = "Logging";
    center=true;

    Logging [label = "grpc-client-logging", shape=box, width=2.0 ];
  }

  PreCore [label = "PreCore", shape=box, width=2.0];
  Auth [label = "Auth", shape=box, width=2.0];
  PostCore [label = "PostCore", shape=box, width=2.0];


  Baggage -> PostCore [penwidth=3, dir=both  arrowtail=none];
  PostCore -> DeadlinePropagation [penwidth=3, dir=both  arrowtail=none];
  DeadlinePropagation -> Auth [penwidth=3, dir=both  arrowtail=none];
  Auth -> Logging [penwidth=3, dir=both  arrowtail=none];
  Logging -> PreCore [penwidth=3, minlen=0, dir=both arrowtail = none];

  Pipeline[label = "grpc-client-middlewares-pipeline", shape=plaintext, rank="main"];
}
@enddot

## MiddlewareFactoryComponentBase

There are two ways to implement a middleware component:
1. A short-cut `ugrpc::client::SimpleMiddlewareFactoryComponent`
2. `ugrpc::client::MiddlewareFactoryComponentBase`

Best practice:

If your middleware doesn't use static config options, just use a short-cut.

@warning In that case, `kName` and `kDependency` (like `middlewares::MiddlewareDependencyBuilder`) must be in a Middleware class (as shown above).

If you want to use static config options for your middleware, use @ref ugrpc::client::MiddlewareFactoryComponentBase. See @ref scripts/docs/en/userver/grpc/middlewares_configuration.md

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/grpc/server_middlewares.md | @ref scripts/docs/en/userver/grpc/middlewares_order.md ⇨
@htmlonly </div> @endhtmlonly
