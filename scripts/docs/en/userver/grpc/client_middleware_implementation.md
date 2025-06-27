# gRPC client middleware implementation


## Two main classes

There are two main interfaces for implementing a middleware:
1. @ref ugrpc::client::MiddlewareBase. Class that implements the main logic of a middleware.
2. @ref ugrpc::client::SimpleMiddlewareFactoryComponent short-cut for simple cases without static options. 
    * Or @ref ugrpc::client::MiddlewareFactoryComponentBase to declare static options.

## MiddlewareBase

@ref ugrpc::client::MiddlewareBase

### PreStartCall and PostFinish

Methods @ref ugrpc::client::MiddlewareBase::PreStartCall and @ref ugrpc::client::MiddlewareBase::PostFinish are called once per grpc Call (RPC).

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

#### Per-call (RPC) hooks implementation example

@snippet samples/grpc_middleware_service/src/middlewares/client/auth.hpp Middleware declaration
@snippet samples/grpc_middleware_service/src/middlewares/client/auth.cpp gRPC middleware sample - Middleware implementation

Register the Middleware component in the component system.

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

#### Per-message hooks implementation example

@snippet grpc/src/ugrpc/client/middlewares/log/middleware.hpp MiddlewareBase example declaration
@snippet grpc/src/ugrpc/client/middlewares/log/middleware.cpp MiddlewareBase Message methods example

The static YAML config and component registration are identical as in the example above. So, let's not focus on this.

## MiddlewareFactoryComponent

We use a simple short-cut @ref ugrpc::client::SimpleMiddlewareFactoryComponent in the example above. 
To declare static config options of your middleware see @ref scripts/docs/en/userver/grpc/middlewares_configuration.md.

## Exceptions and errors in middlewares

To fully understand what happens when middlewares hooks are failed, you should understand the middlewares order:
@see @ref grpc_client_middlewares_order.

All exceptions are rethrown to the user code from client's RPC creating methods, `Read` / `Write` (for streaming), and from methods that return the RPC status.

## Using static config options in middlewares

There are two ways to implement a middleware component. You can see above @ref ugrpc::client::SimpleMiddlewareFactoryComponent. This component is need
for simple cases without static config options of a middleware.

@note In that case, `kName` and `kDependency` (@ref middlewares::MiddlewareDependencyBuilder) must be in a middleware class (as shown above).

If you want to use static config options for your middleware, use @ref ugrpc::client::MiddlewareFactoryComponentBase. 
@see @ref scripts/docs/en/userver/grpc/middlewares_configuration.md.

To override static config options of a middleware per a client see @ref grpc_middlewares_config_override.


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

    Baggage [label = "grpc-client-baggage", shape=box, width=2.0];
    HeadersPropagator [label = "grpc-client-headers-propagator", shape=box, width=2.0];
    Testsuite [label = "grpc-client-testsuite", shape=box, width=2.0];

    Baggage -> HeadersPropagator -> Testsuite;
  }

  subgraph cluster_Core {
    shape=box;
    label = "Core";
    center=true;
    rankdir=LR;

    DeadlinePropagation [label = "grpc-client-deadline-propagation", shape=box, width=2.0];
  }

  subgraph cluster_Logging {
    shape=box;
    label = "Logging";
    center=true;

    Logging [label = "grpc-client-logging", shape=box, width=2.0];
  }

  PreCore [label = "PreCore", shape=box, width=2.0];
  Auth [label = "Auth", shape=box, width=2.0];
  PostCore [label = "PostCore", shape=box, width=2.0];

  PreCore -> Logging -> Auth -> DeadlinePropagation -> PostCore -> Baggage;

  Pipeline[label = "grpc-client-middlewares-pipeline\n from the start to the end", shape=plaintext, rank="main"];
}
@enddot


@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/grpc/client_middlewares.md | @ref scripts/docs/en/userver/grpc/middlewares_order.md ⇨
@htmlonly </div> @endhtmlonly

