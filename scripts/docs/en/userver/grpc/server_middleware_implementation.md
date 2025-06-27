# gRPC server middleware implementaion

## Two main classes

There are two main interfaces for implementing a middleware:
1. @ref ugrpc::server::MiddlewareBase. Class that implements the main logic of a middleware
2. @ref ugrpc::server::SimpleMiddlewareFactoryComponent short-cut for simple cases without static options. 
    * Or @ref ugrpc::server::MiddlewareFactoryComponentBase to declare static options.

## MiddlewareBase

@anchor grpc_server_hooks
### OnCallStart and OnCallFinish

Methods @ref ugrpc::server::MiddlewareBase::OnCallStart and @ref ugrpc::server::MiddlewareBase::OnCallFinish are called once per grpc Call (RPC).

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
      ReceiveMessages [label = "Receive messages", shape=box];
      HandleRPC [label = "Handle RPC (user gRPC-service code)", shape=box];
      SendMessages [label = "Send messages", shape=box];
    }
  }
  ReceiveMessages -> HandleRPC -> SendMessages

  FirstMiddlewareOnCallStart -> SecondMiddlewareOnCallStart;
  SecondMiddlewareOnCallStart -> ReceiveMessages;
  SendMessages -> SecondMiddlewareOnCallFinish;
  SecondMiddlewareOnCallFinish -> FirstMiddlewareOnCallFinish;

  // fake edges and `invis` is need for a good appearance
  SendMessages -> HandleRPC [style=invis];
  HandleRPC -> ReceiveMessages [style=invis];

  Pipeline[label = "OnCallStart/OnCallFinish middlewares hooks order", shape=plaintext, rank="main"];
}
@enddot

#### Per-Call (RPC) hooks implementation example

@snippet samples/grpc_middleware_service/src/middlewares/server/auth.hpp Middleware declaration
@snippet samples/grpc_middleware_service/src/middlewares/server/auth.cpp Middleware implementation

Register the component.

@snippet samples/grpc_middleware_service/main.cpp gRPC middleware sample - components registration

The static YAML config.

@snippet samples/grpc_middleware_service/configs/static_config.yaml grpc-server-auth static config

### PostRecvMessage and PreSendMessage

You can add some behavior on each request/response. Especially, it can be important for grpc-stream. See about streams in @ref scripts/docs/en/userver/grpc/grpc.md.

`PostRecvMessage` hooks are called in the direct middlewares order. `PreSendMessage` hooks are called in the reversed order.

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

    AcceptMessage [label = "Read a request message", shape=box];
    ReturnMessage [label = "Write a response message", shape=box];
  }

  ReadMessageFromNetwork -> FirstMiddlewarePostRecvMessage -> SecondMiddlewarePostRecvMessage -> AcceptMessage
  ReturnMessage -> SecondMiddlewarePreSendMessage -> FirstMiddlewarePreSendMessage -> WriteMessageToNetwork

  Pipeline[label = "PostRecvMessage/PreSendMessage middlewares hooks order", shape=plaintext, rank="main"];
}
@enddot

For more information about the middlewares order:
@see @ref scripts/docs/en/userver/grpc/middlewares_order.md.

#### Per-message hooks implementation example

@snippet grpc/functional_tests/middleware_server/src/my_middleware.hpp gRPC CallRequestHook declaration example
@snippet grpc/functional_tests/middleware_server/src/my_middleware.cpp gRPC CallRequestHook impl example

The static YAML config.

@snippet grpc/functional_tests/middleware_server/static_config.yaml my-middleware-server config

Register the middleware component in the component system.

@snippet grpc/functional_tests/middleware_server/src/main.cpp Register middlewares


## MiddlewareFactoryComponent

We use a simple short-cut @ref ugrpc::server::SimpleMiddlewareFactoryComponent in the example above. 
To declare static config options of your middleware see @ref scripts/docs/en/userver/grpc/middlewares_configuration.md.

## Exceptions and errors in middlewares

To fully understand what happens when middlewares hooks fail, you should understand the middlewares order:
@see @ref grpc_server_middlewares_order.

If you want to interrupt a Call (RPC) in middlewares, you should use @ref ugrpc::server::MiddlewareCallContext::SetError (see examples above on this page).

If you throw an exception in middlewares hooks, that exception will be translated to `grpc::Status` (by default `grpc::StatusCode::UNKNOWN`) and next hooks won't be called.
@ref server::handlers::CustomHandlerException is translated to a relevant `grpc::Status`. 

All errors will be logged just like an exception or error status from the user handler:

* error status is written in a separate log message by @ref ugrpc::server::middlewares::log::Component before sending it to the upstream client;
* status summary is additionally attached to the span in `error_msg` tag.

@note But exceptions are not the best practice in middlewares hooks ⇒ prefer `SetError`.

### Errors and OnCallFinish

@ref ugrpc::server::MiddlewareBase::OnCallFinish will be called **despite of any errors**. 

The actual status is passed to `OnCallFinish` hooks. Each `OnCallFinish` hook gets the status from a previous `OnCallFinish` call and can change that by `SetError` (or exception). 
An error status from a handler will be passed to a first `OnCallFinish` and that hook can change that status, next hooks will get the new status. If all `OnCallFinish` hooks don't change the status, that status will be the final status for a client.

### The call path example with errors in the pipeline

There are 3 middlewares `A`, `B`, `C`.
Cases:
1. `A::OnCallStart` and `B::OnCallStart` succeed, but `C::OnCallStart` fails (by `SetError` or exception) ⇒ `B::OnCallFinish` and `A::OnCallFinish` will be called (remember that `OnCallFinish` order is reversed).
2. If all `OnCallStart` succeed and `C::OnCallFinish` fails, `B::OnCallStart` and `A::OnCallStart` will be called and these hooks get an error status from `C::OnCallFinish`.
3. If a handler returns an error, all `OnCallFinish` will be called.
4. If there are errors in `PostRecvMessage`/`PreSendMessage` ⇒ RPC is failed ⇒ all `OnCallFinish` hooks will be called.

## Using static config options in middlewares

There are two ways to implement a middleware component. You can see above @ref ugrpc::server::SimpleMiddlewareFactoryComponent. This component is needed
for simple cases without static config options of a middleware.

@note In that case, `kName` and `kDependency` (@ref middlewares::MiddlewareDependencyBuilder) must be in a middleware class (as shown above).

If you want to use static config options for your middleware, use @ref ugrpc::server::MiddlewareFactoryComponentBase. 
@see @ref scripts/docs/en/userver/grpc/middlewares_configuration.md.

To override static config options of a middleware per a server see @ref grpc_middlewares_config_override.


@anchor grpc_server_middlewares_order
## Middleware order

Before starting to read specifics of server middlewares ordering:
@see @ref scripts/docs/en/userver/grpc/middlewares_order.md.

There are simple cases above: we just set `Auth` group for one middleware and use a default constructor of `MiddlewareDependencyBuilder` in other middleware.
Here we say that all server middlewares are located in these groups.

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

    Baggage [label = "grpc-server-baggage", shape=box, width=2.0];
    HeadersPropagator [label = "grpc-server-headers-propagator", shape=box, width=2.0];

    Baggage -> HeadersPropagator;
  }

  subgraph cluster_Core {
    shape=box;
    label = "Core";
    center=true;
    rankdir=LR;

    CongestionControl [label = "congestion-control", shape=box, width=2.0];
    DeadlinePropagation [label = "deadline-propagation", shape=box, width=2.0];

    CongestionControl -> DeadlinePropagation;
  }

  subgraph cluster_Logging {
    shape=box;
    label = "Logging";
    center=true;

    AccessLog [label="grpc-server-access-log", shape=box, width=2.0];
    Logging [label = "grpc-server-logging", shape=box, width=2.0];
  }

  PreCore [label = "PreCore", shape=box, width=2.0];
  Auth [label = "Auth", shape=box, width=2.0];
  PostCore [label = "PostCore", shape=box, width=2.0];

  PreCore -> AccessLog -> Logging -> Auth -> CongestionControl;
  DeadlinePropagation -> PostCore -> Baggage;

  Pipeline[label = "grpc-server-middlewares-pipeline\n from the start to the end", shape=plaintext, rank="main"];
}
@enddot


@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/grpc/server_middlewares.md | @ref scripts/docs/en/userver/grpc/client_middlewares.md ⇨
@htmlonly </div> @endhtmlonly
