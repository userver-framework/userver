# gRPC Timeouts and Retries

## QOS Config

A QOS @ref scripts/docs/en/userver/dynamic_config.md "dynamic config" is a map from a full method name
(`namespace.of.Service/Method`) to the settings for that method. It has the
type @ref ugrpc::client::ClientQos and usually provided to the @ref ugrpc::client::SimpleClientComponent constructor.

There is also a `__default__`, which is the default for all the missing methods and usually used for most
"lightweight" methods of the service.

`__default__` is NOT recursively merged with per-method settings: if a key for a specific method exists, `__default__`
is ignored for it.

Carefully check that you have entered the full method name correctly. There is currently no protection against typos
in this area; an invalid method is silently ignored.


## Timeouts

If the `timeout-ms` field is not specified for the current request, the timeout is considered infinite.

In a request, in addition to `timeout-ms`, the
@ref scripts/docs/en/userver/deadline_propagation.md "deadline of the current handler" is also considered; the minimum
of the two is used.

### Behavior for Streaming RPC

Timeouts apply to the entire RPC, from the moment the RPC is created until it is closed. To prevent the stream from
being randomly terminated, override the QOS for streaming RPCs to an infinite timeout (by omitting the `timeout-ms`
field).

There is no option to configure a timeout for each individual message in a stream
(from sending a request to receiving the corresponding response). Instead, it is common practice in gRPC to use
keepalive pings to ensure that the server is still alive:

* [About keepalive](https://grpc.io/docs/guides/keepalive/)
* [channel-args keys for keepalive](https://grpc.github.io/grpc/core/group__grpc__arg__keys.html#gabeeccb441a671122c75384e062b1b91b)
* In userver, they can be set in the `channel-args` static config option of @ref ugrpc::client::ClientFactoryComponent.

## Retries

For unary RPCs, clients automatically retry calls that fail with an error.

Conditions for a retry to occur:
1. The error code is one of the retryable status codes:
   @snippet grpc/src/ugrpc/client/impl/retry_policy.cpp  retryable
2. The number of previous attempts is less than `MaxAttempts`;
3. The overall Deadline (including from @ref scripts/docs/en/userver/deadline_propagation.md) has not been exceeded.

### Configuring Retries

For non-codegenerated clients, retries are disabled by default. They can be enabled in any of the ways listed below.

The maximum number of attempts can be configured as follows, in order of increasing priority:
* Via static config:
  @snippet grpc/functional_tests/middleware_client/static_config.yaml grpc client factory
* Via dynamic gRPC QOS config @ref ugrpc::client::ClientQos;
* Via an option in the code:
  @snippet grpc/tests/retry_test.cpp  SetAttempts

Retries can be disabled by specifying 1 attempt.

### Retry Observability

The following tags are set in the call span:

* `max_attempts=<MaxAttempts>` (the limit on the number of attempts)
* `attempts=<attempts performed>` (the number of attempts actually made)

The following tags are also added to all client logs:

* `max_attempts=<MaxAttempts>` (the limit on the number of attempts)
* `attempts=<current attempt>` (the number of the current attempt, starting from 1)


Middlewares are run for each attempt, and logs are written, including a log with an error status, if any.

The client span is shared across all attempts. With retries, a single client span in the tracing system will have
multiple child spans from the downstream service's handler.

### grpc-core Retries {#retries-transparent}

By default, ugrpc leaves the default retry behavior of grpc-core as is, without any additional configuration.
This means that only low-level HTTP/2 errors will be retried (such retries are called Transparent retries).

You can set your own settings for grpc-core retries via the static config:

@snippet grpc/functional_tests/lowlevel/static_config.yaml  retry configuration

You can read more about *grpc-core* retry settings in the official documentation (https://grpc.io/docs/guides/retry/).

### Comparison of userver-grpc and grpc-core retry capabilities

| Feature                       | userver-grpc retries                                  | grpc-core retries                                                    |
| ----------------------------- | ----------------------------------------------------- | -------------------------------------------------------------------- |
| *QOS*                         | +                                                     | -                                                                    |
| *static config*               | `retry-config.attempts`                               | default-service-config (grpc-core format)                            |
| *middlewares*                 | hooks are called on every retry attempt               | hooks are called once for the entire RPC                             |
| *observability*               | each attempt is written separately to logs and metrics, span tags are available | no information about intermediate attempts |
| *retriable status codes*      | hardcoded list                                        | can be configured via static config                                  |
| *gRPC call becomes committed* | retries are performed regardless of response metadata | No further retries will be attempted                                 |


### When grpc-core retries break

Conditions for a retry to occur:

1. grpc-core retries are manually configured in the config. ugrpc automatically configures them for streaming RPCs.
2. The number of attempts is less than the limit.
3. The returned status code is in the configured list of retryable codes. Some HTTP/2 level errors are also retried,
  specifically, `RST_STREAM(REFUSED_STREAM)`, but not `RST_STREAM(INTERNAL)`.
4. The downstream service did not return initial metadata.

See [When Retries are Valid](https://github.com/grpc/proposal/blob/master/A6-client-retries.md#when-retries-are-valid)
for more info.

@warning Do not use initial metadata (headers) unless absolutely necessary. It breaks grpc-core retries. This
         limitation does not apply to trailing metadata (trailers).

It was also discovered experimentally that a Python grpc-io server causes retries to be aborted if the status is
returned as follows:

@code{.py}
context.set_code(grpc.StatusCode.UNAVAILABLE)
context.set_message('Foo')
return my_package_pb2.MyMethodResponse()
@endcode

To prevent this issue, you should do this instead:

@code{.py}
await context.abort(grpc.StatusCode.UNAVAILABLE, 'Foo')
@endcode


### Why we decided to move away from grpc-core retries for unary calls and create our own ugrpc retries

1. Limitations on metadata, see above.
2. The default timeout field in the client's gRPC service config does not allow retrying timeouts. For retrying
   timeouts, there is an experimental `perAttemptTimeoutMs` field. But after we started using this option,
   [it turned out](https://github.com/grpc/grpc/issues/39935) that grpc-core sometimes encounters a race condition
   and hangs indefinitely.


----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/grpc/grpc.md | @ref scripts/docs/en/userver/grpc/server_middlewares.md ⇨
@htmlonly </div> @endhtmlonly

