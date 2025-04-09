# gRPC middlewares order

How to implement server and client middlewares:
1. @ref scripts/docs/en/userver/grpc/server_middlewares.md
2. @ref scripts/docs/en/userver/grpc/client_middlewares.md

## Middlewares pipeline

There is a main component that manages a middlewares pipeline.

1. @ref ugrpc::server::MiddlewarePipelineComponent for the server side
2. @ref ugrpc::client::MiddlewarePipelineComponent for the client side

These components allow you to globally enable/disable middlewares. There isn't one explicit global ordered list of middlewares. 
That approach is not flexible and not scalable. Instead, each middleware can provide an information of an order relative to other middlewares. 
Each middleware sets some dependency order relative to other middlewares using @ref middlewares::MiddlewareDependencyBuilder.
Then Pipeline collects that set of dependencies between middlewares and builds DAG (directed acyclic graph). Then Pipeline builds an **ordered middlewares list** from that DAG.

You can enable/disable middleware per service or per client. See @ref scripts/docs/en/userver/grpc/middlewares_configuration.md

## Middlewares groups

Each middleware belongs to some group. You need to start defining the order of middleware from a group of that middleware.

Groups:

1. @ref middlewares::groups::PreCore
2. @ref middlewares::groups::Logging
3. @ref middlewares::groups::Auth
4. @ref middlewares::groups::Core
5. @ref middlewares::groups::PostCore
6. @ref middlewares::groups::User

The pipeline calls middlewares in that order. `User` group is called the last one. `User` group is the default implicit group for all middlewares.
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
    
    Baggage [label = "baggage", shape=box, width=2.0 ];

    UserMiddlewareB -> UserMiddlewareA [penwidth=3, minlen=0, dir=both arrowtail = none];
    UserMiddlewareA -> Baggage [penwidth=3, minlen=0, dir=both arrowtail = none];
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
    
    Logging [label = "logging", shape=box, width=2.0 ];
  }

  PreCore [label = "PreCore", shape=box, width=2.0];
  Auth [label = "Auth", shape=box, width=2.0];
  PostCore [label = "PostCore", shape=box, width=2.0];


  Baggage -> PostCore [penwidth=3, minlen=0, dir=both arrowtail = none];
  PostCore -> DeadlinePropagation [penwidth=3, dir=both  arrowtail=none];
  Auth -> Logging [penwidth=3, dir=both  arrowtail=none];
  Logging -> PreCore [penwidth=3, minlen=0, dir=both arrowtail = none];
  CongestionControl -> Auth [penwidth=3, dir=both  arrowtail=none];
 
  Pipeline[label = "Pipeline", shape=plaintext, rank="main"];
}
@enddot

There are Post-hooks and Pre-hooks for each middleware interface. Post-hooks are always called in the reverse order. For more info see special implementation of server or client middlewares:
1. @ref grpc_server_hooks
2. @ref grpc_client_hooks

## Defining dependencies of middlewares

If your middleware doesn't care about an order, you can don't construct a builder and pass it to `MiddlewareFactoryComponentBase`. Middleware will be in the User group by default.

```cpp
#include <userver/ugrpc/server/middlewares/base.hpp>

MiddlewareComponent::MiddlewareComponent(const components::ComponentConfig& config, const components::ComponentContext& context)
    : ugrpc::server::MiddlewareFactoryComponentBase(config, context) {}  // is OK.
```

To specify middleware groups and dependencies, you should use `middlewares::MiddlewareDependencyBuilder`.

### InGroup

Each middleware is in the @ref middlewares::groups::User group by default.

If your middleware component is inherited from `ugrpc::{client,server}::MiddlewareFactoryComponentBase`,
middleware can choose some group and pass it in the `MiddlewareFactoryComponentBase` constructor:

@snippet grpc/src/ugrpc/server/middlewares/log/component.cpp middleware InGroup

### Before/After

@snippet samples/grpc_middleware_service/src/middlewares/server/meta_filter.cpp MiddlewareDependencyBuilder After

@warning If your dependencies create a cycle in the middleware graph, there will be a fatal error in the start => be careful.

Now the middleware of MetaFilterComponent will be ordered after `ugrpc::server::middlewares::headers_propagator::Component`. You can create dependencies between middlewares only if they are in the same group.

Also, you can use the method `middlewares::MiddlewareDependencyBuilder::Before`.

@note if there are two (or more) middlewares that not depends on each other, these middlewares will be lexicographically ordered by component names.

## Weak dependency

What happens if some `MiddlewareA` requires `After<MiddlewareB>()` and `MiddlewareB` is disabled? 

If the `After` method was called without arguments, there will be a fatal error at the start of the service.

If you want to skip this dependency in that case, just pass it `middlewares::DependencyType::kWeak` in `After`. Is normal if `MiddlewareA` is really requires `MiddlewareB` to work.

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

`kWeak` is needed for cases where the order of middlewares is need for the best efforts.

## Enable middlewares per service or per client

### The simplest case

You just enable middleware globally by default

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

1. Globaly enable (for all services) and local disable

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
                    enabled: false  # loacly disable
```
2. Globaly disable (for all services) and force local enable

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
                    enabled: true  # force localy enable despite globally disable
```

## How to start a middlewares list from a list from a scratch

There are two additional options in the static config:
1. `disable-all-pipeline-middlewares`
2. `disable-user-pipeline-middlewares`

`disable-all-pipeline-middlewares` may be useful for "system" handlers and clients. E.g. `grpc-health`.

`disable-user-pipeline-middlewares` may be useful for libraries that don't want to receive unknown `User` middlewares to their library middlewares list.

@warning It is dangerous to disable all middlewares! There are userver default middlewares that provide a stability of microservices (e.g., deadline-propagation, congestion-control).

For more info about options see docs `ugrpc::server::ServiceComponentBase` and `ugrpc::client::ClientFactoryComponent`. 

Example of the static YAML config.

```yaml
        grpc-client-factory:
            channel-args: {}
            # Disable all middlewares. But we can force enable some
            disable-all-pipeline-middlewares: true
            middlewares:
                grpc-client-logging:  # force enable
                    enabled: true

```


@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/grpc/client_middlewares.md | @ref scripts/docs/en/userver/grpc/middlewares_configuration.md ⇨
@htmlonly </div> @endhtmlonly
