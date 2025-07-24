# gRPC middlewares order

How to implement server and client middlewares:
1. @ref scripts/docs/en/userver/grpc/server_middlewares.md
2. @ref scripts/docs/en/userver/grpc/client_middlewares.md

## Middlewares pipeline

There is a main component that manages a middlewares pipeline.

1. @ref ugrpc::server::MiddlewarePipelineComponent for the server side
2. @ref ugrpc::client::MiddlewarePipelineComponent for the client side

These components allow you to globally enable/disable middlewares. There isn't one explicit global ordered list of middlewares. 
That approach is not flexible and not scalable. Instead, each middleware can provide an information of position relative to other middlewares. 
Each middleware sets some dependency order relative to other middlewares using @ref middlewares::MiddlewareDependencyBuilder.
Then Pipeline collects that set of dependencies between middlewares and builds DAG (directed acyclic graph). Then Pipeline builds an **ordered middlewares list** from that DAG.

You can enable/disable middleware per service or per client. For more information see @ref scripts/docs/en/userver/grpc/middlewares_toggle.md.

## Middlewares groups

Each middleware belongs to some group. You need to start defining the order of middleware from a group of that middleware.

Groups:

1. @ref middlewares::groups::PreCore
2. @ref middlewares::groups::Logging
3. @ref middlewares::groups::Auth
4. @ref middlewares::groups::Core
5. @ref middlewares::groups::PostCore
6. @ref middlewares::groups::User

The pipeline calls middlewares in that order. `PreCore` group is called first, then `Logging` and so forth. `User` group is called the last one. 
`User` group is the default group for all middlewares.

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
    
    Baggage [label = "baggage", shape=box, width=2.0];

    Baggage -> UserMiddlewareA;
  }

  subgraph cluster_Core {
    shape=box;
    label = "Core";
    center=true;
    rankdir=LR;
    
    CongestionControl [label = "congestion-control", shape=box, width=2.0];
    DeadlinePropagation [label = "deadline-propagation", shape=box, width=2.0];

    DeadlinePropagation -> CongestionControl;
  }

  subgraph cluster_Logging {
    shape=box;
    label = "Logging";
    center=true;
    
    Logging [label = "logging", shape=box, width=2.0];
  }

  subgraph cluster_Auth {
    shape=box;
    label = "Auth";
    center=true;
    
    UserMiddlewareB [label = "UserMiddlewareB", shape=box, width=2.0];
  }

  PreCore [label = "PreCore", shape=box, width=2.0];
  PostCore [label = "PostCore", shape=box, width=2.0];

  PreCore ->Logging -> UserMiddlewareB -> DeadlinePropagation;
  CongestionControl -> PostCore -> Baggage;
 
  Pipeline[label = "Pipeline example", shape=plaintext, rank="main"];
}
@enddot

There are Post-hooks and Pre-hooks for each middleware interface. Post-hooks are always called in the reverse order. For more information see special implementation of server or client middlewares:
1. @ref grpc_server_hooks
2. @ref grpc_client_hooks

## Defining dependencies of middleware

If your middleware doesn't care about an order, your middleware will be in the `User` group by default.

```cpp
#include <userver/ugrpc/server/middlewares/base.hpp>

MiddlewareComponent::MiddlewareComponent(const components::ComponentConfig& config, const components::ComponentContext& context)
    : ugrpc::server::MiddlewareFactoryComponentBase(config, context) {}  // OK.
```

To specify middleware group and dependencies, you should use `middlewares::MiddlewareDependencyBuilder`.

### InGroup

Each middleware is in the @ref middlewares::groups::User group by default.

If your middleware component is inherited from `ugrpc::{client,server}::MiddlewareFactoryComponentBase`,
you can choose some group for that middleware and pass it in the `MiddlewareFactoryComponentBase` constructor:

@snippet grpc/src/ugrpc/server/middlewares/log/component.cpp middleware InGroup

If your middleware component is a short-cut `ugrpc::{server, client}::SimpleMiddlewareFactoryComponent`, you can choose some group with `kDependency` field:

@snippet samples/grpc_middleware_service/src/middlewares/client/auth.hpp Middleware declaration

### Before/After

@snippet samples/grpc_middleware_service/src/middlewares/server/meta_filter.cpp MiddlewareDependencyBuilder After

@warning If your dependencies create a cycle in the middleware graph, there will be a fatal error in the start ⇒ be careful.

Now the middleware of MetaFilterComponent will be ordered after `ugrpc::server::middlewares::headers_propagator::Component`. You can create dependencies between middlewares only if they are in the same group, otherwise 
there will be an exception on the start of the service.

Also, you can use the method `middlewares::MiddlewareDependencyBuilder::Before`. You can use methods `Before`/`After` together and effect will be accumulated.
E.g. `A.Before<B>().After<C>()` ⇒ order from left to right: C, A, B.

@note Middleware are independent of each other, are lexicographically ordered by component names.

E.g. there are one independent middleware and two chains: `A.After<Z>` and `C.After<B>`. Final order is `B, Z, A, C, F`. 

@dot
digraph Pipeline {
  node [shape=box];
  compound=true;
  fixedsize=true;
  rankdir=BT;

  subgraph cluster_DependencyNodes {
    shape=ellipse;
    label = "Dependent nodes (sort these)";
    center=true;
    rankdir=LR;
    
    A [label = "A", shape=circle];
    C [label = "C", shape=circle];
  }

  subgraph cluster_FreeNodes {
    shape=oval;
    label = "Free nodes";
    center=true;
    rankdir=LR;
    
    B [label = "B", shape=doublecircle, width=1.0];
    F [label = "F", shape=doublecircle, width=1.0];
    Z [label = "Z", shape=doublecircle, width=1.0];
  }

  A -> Z [label="  A.After<Z>", fontname="Courier New"];
  C -> B [label="  C.After<B>", fontname="Courier New"];
}
@enddot

## Weak dependency

What happens if some `MiddlewareA` requires `After<MiddlewareB>()` and `MiddlewareB` is disabled? 

If the `After` method was called without arguments, there will be a fatal error at the start of the service.

If you want to skip this dependency in that case, just pass it `middlewares::DependencyType::kWeak` in `After`.

```cpp
#include <userver/ugrpc/server/middlewares/base.hpp>

MiddlewareComponent::MiddlewareComponent(const components::ComponentConfig& config, const components::ComponentContext& context)
    : ugrpc::server::MiddlewareFactoryComponentBase(
          config,
          context,
          middlewares::MiddlewareDependencyBuilder()
              .After<OtherMiddlewareComponent>(middlewares::DependencyType::kWeak)
      ) {}
```

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/grpc/client_middleware_implementation.md | @ref scripts/docs/en/userver/grpc/middlewares_toggle.md ⇨
@htmlonly </div> @endhtmlonly
