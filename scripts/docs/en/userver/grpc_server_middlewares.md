# gRPC server middlewares

## Default middlewares

The gRPC server can be extended by middlewares.
Middleware is called on each incoming (for service) or outgoing (for client) RPC request.
Different middlewares handle the call in the defined order.
A middleware may decide to reject the call or call the next middleware in the stack.
Middlewares may implement almost any enhancement to the gRPC server including authorization
and authentication, ratelimiting, logging, tracing, audit, etc.

There is an `ugrpc::server::MiddlewarePipeline` component for configuring the middlewares pipeline. 
There are default middlewares:
 - grpc-server-logging
 - grpc-server-deadline-propagation
 - grpc-server-congestion-control
 - grpc-server-baggage
 - grpc-server-headers-propagator

All of these middlewares are enabled by default. However, you must register components of these middlewares in the component list.
You should use `ugrpc::server::DefaultComponentList` or `ugrpc::server::MinimalComponentList`.

`ugrpc::server::MiddlewarePipeline` is a global configuration of server middlewares. So, you can enabled/disable middlewares with the option `enabled` in the global (`grpc-server-middlewares-pipeline`) or middleware config.

Example configuration:
```yaml
components_manager:
    components:
        grpc-server:

        grpc-server-middlewares-pipeline:
            middlewares:
                grpc-server-headers-propagator:
                    enabled: false

        some-service:
            middlewares:
                # force enable in this service. Or it can be disabled for special service
                grpc-server-headers-propagator:
                    enabled: true

```

## User middlewares

If you implement your server middleware, you must register it in config `grpc-server-middlewares-pipeline`.

Example:

@snippet samples/grpc_middleware_service/static_config.yaml gRPC middleware sample - static config grpc-server-middlewares-pipeline

Your middleware will be called after all userver middlewares. Your middlewares will be lexicographic ordered.

## Middlewares order

It possible to order middlewares in the pipeline. Use `ugrpc::server::MiddlewareDependencyBuilder` and (optional) enum `ugrpc::server::DependencyType`.

```cpp
#include <userver/ugrpc/server/middlewares/pipeline.hpp>

MiddlewareComponent::MiddlewareComponent(const components::ComponentConfig& config, const components::ComponentContext& context)
    : MiddlewareComponentBase(
          config,
          context,
          ugrpc::server::MiddlewareDependencyBuilder().After<path_to_my_middleware::Component>()
      ) {}

```
Then the middleware of the component `MiddlewareComponent` will be after `path_to_my_middleware::Component` in the pipeline.

@warning Middlewares that ordered with `ugrpc::server::MiddlewareDependencyBuilder::Before` and `ugrpc::server::MiddlewareDependencyBuilder::After` must be in the same group.

If then someone disable middleware `path_to_my_middleware::Component`, userver does not start, because `MiddlewareComponent` requested this middleware. So, this connection is strong (`ugrpc::server::DependencyType::kStrong`). You can set `ugrpc::server::DependencyType::kWeak` to ignore disabling of `path_to_my_middleware::Component`.

```cpp
#include <userver/ugrpc/server/middlewares/pipeline.hpp>

MiddlewareComponent::MiddlewareComponent(const components::ComponentConfig& config, const components::ComponentContext& context)
    : MiddlewareComponentBase(
          config,
          context,
          ugrpc::server::MiddlewareDependencyBuilder()
              .After<path_to_my_middleware::Component>(ugrpc::server::DependencyType::kWeak)
      ) {}
```

@warning The middlewares pipeline can't has a cycles. Userver does not start. So, be careful when order middlewares.

## Middlewares groups

The middlewares pipeline consists of groups (ordered from begin to end):
 - `ugrpc::groups::PreCore`
 - `ugrpc::groups::Logging`
 - `ugrpc::groups::Auth`
 - `ugrpc::groups::Core`
 - `ugrpc::groups::PostCore`
 - `ugrpc::groups::User`

All user middlewares will be in `ugrpc::server::groups::User` group by default. But you can register your middleware in any group such as:

@snippet samples/grpc_middleware_service/src/middlewares/server/middleware.hpp gRPC middleware sample - Middleware declaration

You can don't pass `ugrpc::server::MiddlewareDependencyBuilder` in `ugrpc::server::MiddlewareComponentBase` and middleware will be in the group `User` by default.

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/grpc.md | @ref rabbitmq_driver ⇨
@htmlonly </div> @endhtmlonly
