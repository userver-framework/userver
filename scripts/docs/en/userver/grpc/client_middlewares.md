# gRPC client middlewares

@see @ref scripts/docs/en/userver/tutorial/grpc_middleware_service.md

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
 - @ref ugrpc::client::middlewares::testsuite::Component (used only in testsuite)

If you add these middlewares to the @ref components::ComponentList, these middlewares will be enabled by default. 
To register core gRPC client components and a set of builtin middlewares use @ref ugrpc::client::DefaultComponentList or @ref ugrpc::client::MinimalComponentList.
As will be shown below, custom middlewares require additional actions to work: registering in `grpc-client-middleware-pipeline` and writing a required static config entry.

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

For more information about `enabled`:
@see @ref scripts/docs/en/userver/grpc/middlewares_configuration.md

## Two main classes

There are two main interfaces for implementing a middleware:
1. `ugrpc::client::MiddlewareBase`. Class that implements the main logic of a middleware.
2. `ugrpc::client::MiddlewareFactoryComponentBase` (the factory for the middleware).
    * Or for simple cases @ref ugrpc::client::SimpleMiddlewareFactoryComponent.

## MiddlewareBase

@ref ugrpc::client::MiddlewareBase

### PreStartCall and PostFinish

`PreStartCall` is called before the first message is sent.
`PostFinish` is called after the last message is received or after an error status is received from the downstream service.

`PreStartCall` hooks are called in the direct middlewares order. `PostFinish` hooks are called in the reversed order.

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

    FirstMiddlewarePreStartCall [label = "PreStartCall"];
    FirstMiddlewarePostFinish [label = "PostFinish" ];
  }

  subgraph cluster_SecondMiddleware{
    shape=box;
    label = "SecondMiddleware";
    rankdir=TB;

    SecondMiddlewarePreStartCall [label = "PreStartCall"];
    SecondMiddlewarePostFinish [label = "PostFinish"];
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
      SendMessages [label = "Send messages", shape=box];
      HandleRPC [label = "Handle RPC", shape=box];
      ReceiveMessages [label = "Receive messages", shape=box];
      InvisibleRpcHandlingEnd [shape=plaintext, label="", height=0];

    }
  }
  SendMessages -> HandleRPC -> ReceiveMessages
  // invis is need for a good appearance
  ReceiveMessages -> HandleRPC [style=invis];
  HandleRPC -> SendMessages [style=invis];


  FirstMiddlewarePreStartCall -> SecondMiddlewarePreStartCall;
  SecondMiddlewarePreStartCall -> SendMessages [label = "once"];
  ReceiveMessages -> SecondMiddlewarePostFinish [label = "once"];
  SecondMiddlewarePostFinish -> FirstMiddlewarePostFinish;

  Pipeline[label = "PreStartCall/PostFinish middlewares hooks order", shape=plaintext, rank="main"];
}
@enddot

Streaming RPCs can have multiple requests and responses, but `PreStartCall` and `PostFinish` are called once per RPC in any case.

For more information about the middlewares order:
@see @ref scripts/docs/en/userver/grpc/middlewares_order.md.

These hooks are called once per Call (RPC).

@snippet samples/grpc_middleware_service/src/middlewares/client/auth.hpp Middleware declaration
@snippet samples/grpc_middleware_service/src/middlewares/client/auth.cpp gRPC middleware sample - Middleware implementation

Register the Middleware component in the component system. See `sample::grpc::auth::client::AuthComponent`.

@snippet samples/grpc_middleware_service/main.cpp gRPC middleware sample - components registration

The static YAML config.

@snippet samples/grpc_middleware_service/configs/static_config.yaml static config grpc-auth-client

@anchor grpc_client_hooks
### PreSendMessage and PostRecvMessage

`PreSendMessage` hooks are called in the order of middlewares. `PostRecvMessage` hooks are called in the reverse order of middlewares.

@dot
digraph Pipeline {
  node [shape=box];
  compound=true;
  fixedsize=true;
  rankdir=LR;
  tooltip = "You didn't hit the arrow with the cursor :-)";
  labeljust = "l";
  labelloc = "t";

  subgraph cluster_UserClientCode {
    shape=box;
    label = "User code working with gRPC client";

    CreateMessage [label = "Create a message", shape=box];
    RecvMessage [label = "Receive a message", shape=box];
  }

  subgraph cluster_FirstMiddleware {
    shape=box;
    label = "FirstMiddleware";

    FirstMiddlewareCallRequestHook [label = "CallRequestHook", shape=box];
    FirstMiddlewareCallResponseHook [label = "CallResponseHook", shape=box];
  }

  subgraph cluster_SecondMiddleware{
    shape=box;
    label = "SecondMiddleware";

    SecondMiddlewareCallRequestHook [label = "CallRequestHook", shape=box];
    SecondMiddlewareCallResponseHook [label = "CallResponseHook", shape=box];
  }

  subgraph cluster_NetworkInteraction {
    shape=box;
    label = "Network interaction";

    SendMessageToNetwork [label = "Send message to network", shape=box];
    ReceiveMessageFromNetwork [label = "Receive message from network", shape=box];
  }

  CreateMessage -> FirstMiddlewareCallRequestHook -> SecondMiddlewareCallRequestHook -> SendMessageToNetwork
  ReceiveMessageFromNetwork -> SecondMiddlewareCallResponseHook -> FirstMiddlewareCallResponseHook -> RecvMessage

  Pipeline[label = "PreSendMessage/PostRecvMessage middlewares hooks order", shape=plaintext, rank="main"];
}
@enddot

For more information about the middlewares order:
@see @ref scripts/docs/en/userver/grpc/middlewares_order.md.

These hooks are called on each message.

`PreSendMessage`:
    * unary: is called exactly once
    * stream: is called 0, 1 or more

`PostRecvMessage`:
    * unary: is called 0 or 1 (0 if service doesn't return a message)
    * stream: is called 0, 1 or more

@snippet grpc/src/ugrpc/client/middlewares/log/middleware.hpp MiddlewareBase example declaration
@snippet grpc/src/ugrpc/client/middlewares/log/middleware.cpp MiddlewareBase Message methods example

The static YAML config and component registration are identical as in example above. So, let's not focus on this.

## Exceptions and errors in middlewares

To fully understand what happens when middlewares hooks are failed, you should understand the middlewares order:
@see @ref grpc_client_middlewares_order.

All exceptions are rethrown to the user code from client's RPC creating methods, `Read` / `Write` (for streaming), and from methods that return the RPC status.

@anchor grpc_client_middlewares_order
## Middlewares order

Before starting to read specifics of client middlewares ordering:
@see @ref scripts/docs/en/userver/grpc/middlewares_order.md.

There are simple cases above: we just set `Auth` group for one middleware.

Here we say that all client middlewares are located in these groups.

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

    Baggage [label = "grpc-client-baggage", shape=box];
    Testsuite [label = "grpc-client-testsuite", shape=box];

    Testsuite -> Baggage;
  }

  subgraph cluster_Core {
    shape=box;
    label = "Core";
    center=true;
    rankdir=LR;

    DeadlinePropagation [label = "grpc-client-deadline-propagation", shape=box];
  }

  subgraph cluster_Logging {
    shape=box;
    label = "Logging";
    center=true;

    Logging [label = "grpc-client-logging", shape=box];
  }

  PreCore [label = "PreCore", shape=box];
  Auth [label = "Auth", shape=box];
  PostCore [label = "PostCore", shape=box];

  Baggage -> PostCore -> DeadlinePropagation -> Auth -> Logging -> PreCore;

  Pipeline[label = "grpc-client-middlewares-pipeline", shape=plaintext, rank="main"];
}
@enddot

## MiddlewareFactoryComponentBase

There are two ways to implement a middleware component. You can see above @ref ugrpc::client::SimpleMiddlewareFactoryComponent. This component is need
for simple cases without static config options of a middleware.

@warning In that case, `kName` and `kDependency` (@ref middlewares::MiddlewareDependencyBuilder) must be in a middleware class (as shown above).

If you want to use static config options for your middleware, use @ref ugrpc::client::MiddlewareFactoryComponentBase. 
@see @ref scripts/docs/en/userver/grpc/middlewares_configuration.md.


@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/grpc/server_middlewares.md | @ref scripts/docs/en/userver/grpc/middlewares_order.md ⇨
@htmlonly </div> @endhtmlonly
