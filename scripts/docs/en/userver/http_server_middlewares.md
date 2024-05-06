# Http server middlewares

## Introduction

Middleware is software that's assembled into an app pipeline to handle requests and responses. Each component:

* Chooses whether to pass the request to the next component in the pipeline.
* Can perform work before and after the next component in the pipeline.

🐙 **userver** provides interfaces to implement these pipeline pieces and to configure them
as well as the resulting pipeline for the http server.
The general idea behind middlewares is no different from that in other web-frameworks like
[Django](https://docs.djangoproject.com/en/5.0/topics/http/middleware/),
[ASP.NET Core](https://learn.microsoft.com/en-us/aspnet/core/fundamentals/middleware/?view=aspnetcore-8.0/),
[Gin](https://gin-gonic.com/docs/examples/custom-middleware/),
[Spring](https://docs.spring.io/spring-framework/reference/web/webmvc/mvc-config/interceptors.html),
[Axum](https://docs.rs/axum/latest/axum/middleware/index.html) etc., but the exact implementation details
differ here and there.

If you've already made yourself familiar with how middlewares are implemented in 🐙 **userver**
you may want to skip straight to @ref middlewares_usage_and_configuration "Usage and configuration".

## High level design

Every http request handled by the components::Server is processed in a number of steps:

1. The request is parsed from the byte stream coming from a network connection
2. The request is traced, accounted for, decompressed/rate-limited/authorized/etc if needed, parsed into
   application-requested format (say, JSON), logged if configured to do so, and a bunch of other steps
3. The request is routed into corresponding server::handlers::HttpHandlerBase
4. The response created by the handler is traced, accounted for, logged if configured to do so... you get the idea
5. The response is serialized into byte stream and sent to the wire

Steps 2. and 4. are implemented by a number of logically-independent middlewares, which allows us to
tune/enhance/rearrange things without going into massive refactoring,
and allows our users to adjust the pipeline as they see fit.

An important detail of the middlewares implementation is that technically middleware is not a
@ref scripts/docs/en/userver/component_system.md "Component", but rather a @ref userver_clients "Client":
given M middlewares and H handlers, there will be M x H instances of middlewares in total, M instances for each handler.
Such arguably more complicated approach is justified by the configurability-freedom it provides: instead of somehow
adjusting the middleware behavior in the code (based on a route, for example), one can configure the middleware at
per-handler basis in exactly the same way most things in userver are configured: by a static config.

Middlewares very well might want to interact with the component system of userver, be that for sharing some common
resource (say, global rate-limiting) or for performing database/http queries. Since a middleware is not a component,
it doesn't have a way to lookup it's dependencies from the @ref components::ComponentContext itself, but rather
the dependencies should be injected into a middleware instance. This is where a MiddlewareFactory comes into play.

Every @ref server::middlewares::HttpMiddlewareBase "Middleware" is paired with a corresponding
@ref server::middlewares::HttpMiddlewareFactoryBase "Factory", which is a Component, and is responsible for creating
middleware instances and injecting required dependencies into these instances. It goes like this: when a handler is
constructed, it looks up all the factories specified by the pipeline configuration, requests each factory to create a
middleware instance, and then builds the resulting pipeline from these instances.
So, to emphasize once again: a Middleware instance is not a Component but rather a Client, and there might be multiple
instances of the same Middleware type, but a MiddlewareFactory is a Component, hence is a singleton.

For reference, this is the userver-provided default middlewares pipeline:
@snippet core/src/server/middlewares/configuration.cpp  Middlewares sample - default pipeline

## Caveats

In general, one should be very careful when modifying the response after the downstream part of the pipeline completed,
and the reason for that is simple: the modification could be overwritten by the upstream part. This is especially
apparent when exceptions are involved: consider the case when an exception is propagating through the pipeline, 
and a middleware is adding a header to the response.
Depending on the exception type and where in the pipeline middleware is put, the header may or may not be overridden 
by the ExceptionsHandling middleware.

Due to this, response-modifying middlewares should be either put before ExceptionsHandling middleware for their changes
to reliably take effect if the downstream pipeline threw, or a middleware should handle downstream exceptions itself.
Middlewares also should never throw on their own: it's ok to let the downstream exception pass through, but throwing
from the middlewares logic could lead to some nonsensical responses being sent to the client, up to violating the http
specification.

The default userver-provided middlewares pipeline handles all these cases and is guaranteed to still have all the
tracing headers and a meaningful status code (500, likely) present for the response even if the user-built part of the
middleware pipeline threw some random exception, but if you reorder or hijack the default pipeline, you are on your own.

@anchor middlewares_usage_and_configuration
## Usage and configuration

This is how a minimal implementation of a middleware looks like:
@snippet samples/http_middleware_service/http_middleware_service.cpp  Middlewares sample - minimal implementation
It doesn't have any logic in it and just passes the execution to the downstream.
This is how a factory for this middleware looks:
@snippet samples/http_middleware_service/http_middleware_service.cpp  Middlewares sample - minimal factory implementation
which feels too verbose for the amount of logic the code performs, so we have a shortcut version, which does the same 
and also passes the handler into the middleware constructor. Given the middleware that performs some logic
@snippet samples/http_middleware_service/http_middleware_service.cpp  Middlewares sample - some middleware implementation
the factory implementation is just this:
@snippet samples/http_middleware_service/http_middleware_service.cpp  Middlewares sample - some middleware factory implementation

### Per-handler middleware configuration

Basically, the whole point of having MiddlewareFactory-ies separated from Middleware-s, is to have a possibility to 
configure a middleware at a per-handler basis.
In the snippet above that's what "handler-middleware.header-value" is for: given the middleware (which actually 
resembles pretty close to how tracing headers are set to the response in userver)
@snippet samples/http_middleware_service/http_middleware_service.cpp  Middlewares sample - configurable middleware implementation
and the factory implementation
@snippet samples/http_middleware_service/http_middleware_service.cpp  Middlewares sample - configurable middleware factory implementation
one can configure the middleware behavior (header value, in this particular case) in the handler's static config.

If a global configuration is desired (that is, for every middleware instance there is), the easiest way to achieve that
would be to have a configuration in the Factory config, and for Factory to pass the configuration into the Middleware 
constructor. This takes away the possibility to declare a Factory as a SimpleHttpMiddlewareFactory, but we find this
tradeoff acceptable (after all, if a middleware needs a configuration it isn't that "Simple" already).

## Pipelines configuration

Now, after we have a middleware and its factory implemented, it would be nice to actually use the middleware in the
pipeline.
🐙 **userver** provides two interfaces for configuring middleware pipelines: one for a server-wide configuration,
and one for a more granular per-handler configuration.

### Server-wide middleware pipeline

The server-wide pipeline is server::middlewares::PipelineBuilder. In its simple form, it takes
server::middlewares::DefaultPipeline and appends the given middlewares to it, which looks like this:
@snippet samples/http_middleware_service/static_config.yaml  Middlewares sample - pipeline builder configuration

If a more sophisticated behavior is desired, derive from server::middlewares::PipelineBuilder and override
its `BuildPipeline` method. Then set the custom pipeline component's name in the server config:

```yaml
        server:
            # ...
            middleware-pipeline-builder: custom-pipeline-builder
```

Remember that messing with the default userver-provided pipeline is error-prone and leaves you on your own.

### Custom per-handler middleware pipelines

To configure the pipeline at a per-handler basis 🐙 **userver** provides server::middlewares::HandlerPipelineBuilder interface.
By default, it returns the server-wide pipeline without any modifications to it. To change the behavior one should
derive from it, override the `BuildPipeline` method and specify the builder as the pipeline-builder for the handler.
For example:
@snippet samples/http_middleware_service/http_middleware_service.cpp  Middlewares sample - custom handler pipeline builder
and to use the class as a pipeline builder we should append it to the @ref components::ComponentList "ComponentList"
@snippet samples/http_middleware_service/http_middleware_service.cpp  Middlewares sample - custom handler pipeline builder registration
and specify as a pipeline-builder for the handler (notice the middlewares.pipeline-builder section):
@snippet samples/http_middleware_service/static_config.yaml  Middlewares sample - custom handler pipeline builder configuration
