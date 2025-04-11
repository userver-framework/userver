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

@anchor grpc_server_hooks
### OnCallStart and OnCallFinish

`OnCallStart` is called after the client metadata is received.
`OnCallFinish` is called before the last message is sent or before error status is sent to a client.

`OnCallStart` hooks are called in the order of middlewares. `OnCallFinish` hooks are called in the reverse order of middlewares

@dot
digraph Pipeline {
  node [shape=box];
  compound=true;
  fixedsize=true;
  rankdir=LR;
  tooltip = "You didn't hit the arrow with the cursor :-)";
  labeljust = "l";
  labelloc = "t";

  subgraph cluster_FirstMiddleware {
    shape=box;
    label = "FirstMiddleware";
    rankdir=TB;

    FirstMiddlewareOnCallStart [label = "OnCallStart"];
    FirstMiddlewareOnCallFinish [label = "OnCallFinish" ];
  }

  subgraph cluster_SecondMiddleware{
    shape=box;
    label = "SecondMiddleware";
    rankdir=TB;

    SecondMiddlewareOnCallStart [label = "OnCallStart"];
    SecondMiddlewareOnCallFinish [label = "OnCallFinish"];
  }

  subgraph cluster_RpcHandling {
    shape=box;
    label = "RPC handling";
    rankdir=TB;

    HandleRPC [label = "Handle RPC", shape=box];
  }

  subgraph cluster_RpcHandling {
    shape=box;
    rankdir=TB;

    {
      rank=same;
      // Invisible nodes are necessary for a good appearance
      InvisibleRpcHandlingEmpty [shape=plaintext, label="", height=0];
      ReceiveMessages [label = "Receive messages", shape=box];
      HandleRPC [label = "Handle RPC", shape=box];
      SendMessages [label = "Send messages", shape=box];
      InvisibleRpcHandlingEnd [shape=plaintext, label="", height=0];

    }
  }
  ReceiveMessages -> HandleRPC -> SendMessages

  FirstMiddlewareOnCallStart -> SecondMiddlewareOnCallStart;
  SecondMiddlewareOnCallStart -> ReceiveMessages [label = "once"];
  SendMessages -> SecondMiddlewareOnCallFinish [label = "once"];
  SecondMiddlewareOnCallFinish -> FirstMiddlewareOnCallFinish;

  // fake edges and `invis` is need for a good appearance
  SendMessages -> HandleRPC [style=invis];
  HandleRPC -> ReceiveMessages [style=invis];

  Pipeline[label = "OnCallStart/OnCallFinish middlewares hooks order", shape=plaintext, rank="main"];
}
@enddot

There are methods @ref ugrpc::server::MiddlewareBase::OnCallStart and @ref ugrpc::server::MiddlewareBase::OnCallFinish that are called once per grpc Call (RPC).

@snippet samples/grpc_middleware_service/src/middlewares/server/auth.hpp Middleware declaration

Each middleware must call `ugrpc::server::MiddlewareCallContext::Next` to continue a middleware pipeline.

@snippet samples/grpc_middleware_service/src/middlewares/server/auth.cpp Middleware implementation

Register the component

@snippet samples/grpc_middleware_service/main.cpp server AuthComponent

The static YAML config.

@snippet samples/grpc_middleware_service/configs/static_config.yaml grpc-server-auth static config

### PostRecvMessage and PreSendMessage

`PostRecvMessage` are called in the right order. 'PreSendMessage' are called in the reverse order

@dot
digraph Pipeline {
  node [shape=box];
  compound=true;
  fixedsize=true;
  rankdir=LR;
  tooltip = "You didn't hit the arrow with the cursor :-)";
  labeljust = "l";
  labelloc = "t";

  subgraph cluster_NetworkInteraction {
    shape=box;
    label = "Network interaction";

    ReadMessageFromNetwork [label = "Read a message from network", shape=box];
    WriteMessageToNetwork [label = "Write a message to network", shape=box];
  }

  subgraph cluster_FirstMiddleware {
    shape=box;
    label = "FirstMiddleware";

    FirstMiddlewarePostRecvMessage [label = "PostRecvMessage", shape=box];
    FirstMiddlewarePreSendMessage [label = "PreSendMessage", shape=box];
  }

  subgraph cluster_SecondMiddleware{
    shape=box;
    label = "SecondMiddleware";

    SecondMiddlewarePostRecvMessage [label = "PostRecvMessage", shape=box];
    SecondMiddlewarePreSendMessage [label = "PreSendMessage", shape=box];
  }

  subgraph cluster_UserServiceCode {
    shape=box;
    label = "User gRPC-service code";

    AcceptMessage [label = "Accept a message", shape=box];
    ReturnMessage [label = "Return a message", shape=box];
  }

  ReadMessageFromNetwork -> FirstMiddlewarePostRecvMessage -> SecondMiddlewarePostRecvMessage -> AcceptMessage
  ReturnMessage -> SecondMiddlewarePreSendMessage -> FirstMiddlewarePreSendMessage -> WriteMessageToNetwork

  Pipeline[label = "PostRecvMessage/PreSendMessage middlewares hooks order", shape=plaintext, rank="main"];
}
@enddot

For more info about the middlewares order see @ref scripts/docs/en/userver/grpc/middlewares_order.md

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
For more info about the middlewares order see @ref scripts/docs/en/userver/grpc/middlewares_order.md

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

    Baggage [label = "grpc-server-baggage", shape=box];
    HeadersPropagator [label = "grpc-server-headers-propagator", shape=box];


    HeadersPropagator -> Baggage;
  }

  subgraph cluster_Core {
    shape=box;
    label = "Core";
    center=true;
    rankdir=LR;

    CongestionControl [label = "congestion-control", shape=box];
    DeadlinePropagation [label = "deadline-propagation", shape=box];

    DeadlinePropagation -> CongestionControl;
  }

  subgraph cluster_Logging {
    shape=box;
    label = "Logging";
    center=true;

    Logging [label = "grpc-server-logging", shape=box];
  }

  PreCore [label = "PreCore", shape=box];
  Auth [label = "Auth", shape=box];
  PostCore [label = "PostCore", shape=box];


  Baggage -> PostCore -> DeadlinePropagation;
  Auth -> Logging -> PreCore;
  CongestionControl -> Auth;

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
