# Deadline propagation

## Theory

With deadline propagation, the timeout of each specific request is set not statically, but depending on the time
remaining to process the calling service's request.

If the service understands that the parent request is to time out sooner than the other timeouts could fire, then it
performs the request with the time remaining to process the parent request. Because of this, after
the deadline is reached, no extra time is spent on processing the request, for which no one is waiting anymore.

In general, the deadline propagation feature starts working and significantly helps to save CPU resources only when
requests are massively canceled by deadline, that is, when the CPU is overloaded, Congestion Control is turned on, some
hosts or services are failing, and so on. That is, it is a mechanism for increasing the stability of multiservice
architectures.

### A classic example of deadline propagation

Let's say, for example, that a request goes through the following services:

`A -> B -> C`

Let the static timeout of the service **A** be 20 seconds, **B** is 15 seconds, **C** is 10 seconds.

Without deadline propagation, request processing may look like this:

* **A** computes something for 12 seconds
* **A** sends a request to **B**
    * **B** computes something for 8 seconds
* The result of the request chain is sure to be discarded by the caller of **A**
* **B** continues to compute something for another 4 seconds (pointlessly)
* **B** sends a request to **C** (pointlessly)
* ...
* Results from the request chain are thrown out

With deadline propagation:

* **A** sends a deadline to **B**, according to which **B** has 8 seconds left
* **B** computes something for 8 seconds
* **B** interrupts pointless request processing, the end

See also [Google's guide](https://sre.google/sre-book/addressing-cascading-failures).

### Why doesn't "close propagation" work

`A -> B -> C`

How canceling requests through connection closure would supposedly work:

* on the timeout in **A**, it breaks the connection with **B**
* this cancels the request task in **B**
* this cancels the request to the client and breaks the connection with **C**
* this cancels the request task in **C**

In practice, it won't work that way:

* closing and reopening connections is expensive; we try to keep connections and send a stream of requests through them
  as long as possible
* closing the client connection may be disabled by proxies like nginx, the client will not know about the cancellation
  in the calling service
* "close propagation" is not instantaneous, it can take a long time to get from **A** to **C**

### How it works in userver

In HTTP, we use custom headers, as described below.

In gRPC, we use the built-in deadline mechanism based on the `grpc-timeout` header.

Database clients use timeout fields specific for them.

**Summary:**

* In the client that initiates the request chain:

    * Set the deadline header based on the timeout value, initiating the deadline propagation process

* In handlers:

    * Set the task-inherited deadline from the header
    * If deadline expires by the end of handling, replace the response with a special "Deadline expired" response

* In clients:

    * Immediately return an error if the deadline has already expired
    * Use the task-inherited deadline to update the timeout
    * Propagate the deadline further as a header

* In database clients:

    * Stop waiting for a free connection and immediately return an error if the deadline has already expired
    * Use task-inherited deadline to update the timeout
    * Include the deadline in the database query

### Why is `duration` passed and not `time_point`

The engine uses a deadline (i.e. `time_point`) to measure time, and it is transmitted between hosts as a timeout
(i.e. `duration`). This approach has the drawback it does not take into account RTT between services.

The decision to transmit the `duration` was made based on the fact that the clocks may not be synchronized accurately
enough between hosts. In case of an unfortunate combination of circumstances, the service may reject the request
prematurely. This problem especially affects requests with small timeouts.

### Deadline Propagation HTTP headers

The deadline propagation mechanism in userver uses custom headers.

What follows are the minimum semantic requirements for services that interact with userver-based services over HTTP to
support deadline propagation.

1. `X-YaTaxi-Client-TimeoutMs`

    - The client sends this header in the request, indicating the deadline for the called service

    - The deadline for the called service may be the same or be less than the deadline for the calling service

2. `X-YaTaxi-Deadline-Expired: <any non-empty value>`

    - The server includes this header in the response if the request could not be processed before the
      deadline from header (1)

    - This header must follow an HTTP status code in the range 400-599

    - In some cases, such a response may be sent before the actual deadline, if the server estimates that it is unable
      to respond in time before the specified deadline

    - When receiving such a response, the HTTP client in the calling service should process it as follows:

        - Process such a response like timeouts or cancellations, discard the response body

        - If the request was made using the entire remainder of the calling service's deadline:
            - Do not retry (regardless of the received HTTP status code)
            - The current handler should itself answer `Deadline expired`

        - If the request was NOT made using the entire remainder of the calling service's deadline:
            - Interpret the response like a typical timeout (regardless of the received HTTP status code and body)
            - Apply the rules for retrying a timeout

## API

### Blocking the deadline propagation

Task-inherited deadline is by default propagated from the handler task to child tasks created via `utils::*Async*`.
There it is used in all clients that support it. This is implemented via `server::request::kTaskInheritedData`
and `server::request::GetTaskInheritedDeadline`.

In _background tasks_ that are started from the task of the request, but do not affect its completion, the deadline
should not be propagated from the request tasks. Blocking such deadline propagation can be achieved by the following
mechanisms:

- `concurrent::BackgroundTaskStorage::AsyncDetach`
- `utils::AsyncBackground`
- `engine::AsyncNoSpan` (don't use it if you are not sure that you need it!)

@warning when creating background tasks via `utils::Async` (instead of `utils::AsyncBackground`), requests performed
in them will be interrupted along with the parent task**

In some cases, it makes sense to ignore the inherited deadline and complete the request, even if no one is waiting for
response from the current handle. To do this, make such a request in the scope of
a `server::request::DeadlinePropagationBlocker`.

## Deadline propagation details for HTTP handlers

If there is a header `X-YaTaxi-Client-timeoutMs` in the request, the handler:

* Sets `server::request::TaskInheritedData::deadline`, which is then used in clients

* If the deadline has expired by the time the handler returns, then:

    * the code `498 Deadline Expired` is returned;
        * this code can be configured, see below;
    * the body is `Deadline expired`.

* If by the time the request has been read to the end and the request has started to be processed, the deadline has
  already expired, then the user code is never called, and the handler responds as shown above

### Monitoring

Metrics:

* `deadline-received` (monotonic counter) - counts requests that have a deadline specified;
* `cancelled-by-deadline` (monotonic counter) - counts requests the handling of which was cancelled by deadline
  (deadline expired by the end of handling, or some operation estimated that the deadline would surely expire).

Log tags of the request's `tracing::Span`:

* `deadline_received_ms=...` if the calling service has set a deadline for the request
* if deadline expired while handling the request:
    * `cancelled_by_deadline=1`;
    * `dp_original_body` - the user-provided response body (if any) that was replaced by `Deadline expired`;
    * `dp_original_body_size` - the size of this body in bytes.

### Configuring

To disable deadline propagation in the static config:

- `server.listener.handler-defaults.deadline_propagation_enabled: false`
- or per handler: `<handle component>.deadline_propagation_enabled: false`

To disable deadline propagation in the dynamic config:

- set @ref USERVER_DEADLINE_PROPAGATION_ENABLED to `false`

The default HTTP status code for `Deadline expired` responses is a custom userver-specific `498 Deadline Expired` code.
The code is deliberately chosen in the 4xx range, because it is not a server error by itself. Given infinite time,
the server would probably handle the request successfully.
However, some environments may fail to handle a non-standard code, in which case you may want to configure it.

To configure HTTP status code for `Deadline expired` responses:

- `server.listener.handler-defaults.deadline_expired_status_code: 504`
- or per handler: `<handle component>.deadline_expired_status_code: 504`

## Deadline propagation details for gRPC service implementations

The mechanism works similar to HTTP handlers. The deadline set in the context of the gRPC client is automatically passed
to the context of the gRPC service. If there is a deadline:

* Sets task-inherited deadline, which is then used in clients.

If the deadline has already expired by the time the request is processed, then:

* Request processing is interrupted;

* The response `grpc::statusCode::DEADLINE_EXCEEDED` is returned with the
  message `Deadline propagation: Not enough time to handle this call`.

Checking the deadline when performing streaming `Read` or `Write` operations is not yet implemented.

### Monitoring

Metrics:

* `grpc.server.by-destination.deadline-propagated {grpc_destination=SERVICE_NAME/METHOD_NAME}` (RATE) - counts calls
  with a set deadline;
* `grpc.server.by-destination.cancelled-by-deadline-propagation {grpc_destination=SERVICE_NAME/METHOD_NAME}` (RATE) -
  counts calls for which the RPC was canceled by deadline.

Log tags of the request's `tracing::Span`:

* `deadline_received_ms=...` if the calling client has set a deadline for the request (deadlines over a year are not
  taken into account);
* `cancelled_by_deadline=1` if the request processing was interrupted by the deadline.

### Configuring

To disable deadline propagation in the static config:

* remove `deadline_propagation` from the list of `middlewares` of components of services

To disable deadline propagation in the dynamic config:

* set @ref USERVER_DEADLINE_PROPAGATION_ENABLED to `false`

## Deadline propagation details for the HTTP client

If there is a task-inherited deadline, the client:

1. Uses it as the upper limit of the timeout, in addition to the standard timeout setting
2. If the deadline has already expired, the client immediately throws `clients::http::CancelException`
3. Sets the request header `X-YaTaxi-Client-TimeoutMs` from the timeout (regardless of whether it was decreased through
   deadline propagation)
4. If the task-inherited deadline was reached while working with the request, then:
    - gives the user code `clients::http::CancelException`
    - in the background, it waits for a response up to the standard timeout to try to save the connection
    - sends `server::request::DeadlineSignal` to the current handle that we cancelled the request because of DP
5. Otherwise, if the response has `X-YaTaxi-Deadline-Expired` header, it converts the response
   to `clients::http::TimeoutException`

### Monitoring

Metrics:

* `timeout-updated-by-deadline` (monotonic counter) - counts requests for which the deadline was set and affected the
  timeout (while the request was not necessarily canceled by the deadline)
* `cancelled-by-deadline` (monotonic counter) - counts requests which was cancelled by the task-inherited deadline

Log tags of the request's `tracing::Span`:

* `propagated_timeout_ms` - if the deadline was set and affected the timeout (while the request was not necessarily
  canceled by the deadline)
* `cancelled_by_deadline=1` - if the request was canceled by the task-inherited deadline

### Configuring

To disable deadline propagation in the static config:

* `http-client.set-deadline-propagation-header: false`
* timeouts are updated from the task-inherited deadline regardless of this config

To disable deadline propagation in the dynamic config:

* set @ref USERVER_DEADLINE_PROPAGATION_ENABLED to `false`

## Deadline propagation details for gRPC clients

If there is a task-inherited deadline, the client uses it as an upper bound for the built-in RPC deadline as implemented
by grpc++.

### Monitoring

Metrics:

* `grpc.client.by-destination.deadline-propagated {grpc_destination=SERVICE_NAME/METHOD_NAME}`
  (RATE) - RPCs for which the original deadline was overridden by the propagated deadline
  (while the request was not necessarily canceled by deadline);

* `grpc.client.by-destination.cancelled-by-deadline-propagation {grpc_destination=SERVICE_NAME/METHOD_NAME}`
  (RATE) - RPCs that were canceled due to the propagated deadline.

Request span Tags:

* `deadline_updated=1` - if the RPC deadline was overridden by the task-inherited deadline;

* `timeout_ms=...` - the final deadline value, represented as a timeout.

### Configuring

To disable deadline propagation in the static config:

* remove `deadline_propagation` from the list of `middlewares` of components of gRPC services or clients

To disable deadline propagation in the dynamic config:

* set @ref USERVER_DEADLINE_PROPAGATION_ENABLED to `false`

## Deadline propagation details for Mongo

1. If the deadline has expired at the start of the request to mongo, or expired before a free connection
   appears, `stores::mongo::CancelledException` is thrown
2. If `maxTimeMS` is not set or is less strict than the deadline, then `maxTimeMS` is updated
    * If the request times out after that, then the usual mongo timeout exception is thrown
      (`stores::mongo::ClusterUnavailableException`)

### Monitoring

If the deadline has expired before the request is actually sent, then:

* metrics indicate error type `cancelled`
* logs include the tag `cancelled_by_deadline=true`

If the request is sent with deadline propagation enabled, then:

* the tag `timeout_ms` is included in the log - it is the time remaining until the deadline expires at the request
  initiation
    * this time includes the time spent waiting for a free connection (if any)
* the tag `max_time_ms` is included in the log - it corresponds to the value of `maxTimeMS` (only included for those
  request types where `maxTimeMs` is allowed)

## Deadline propagation details for Postgres

1. If the deadline has expired before the start of `Execute`, the exception `stores::postgres::ConnectionInterrupted` is
   thrown
2. The request timeout is not updated from the deadline **TODO**

## Deadline propagation details for Redis

1. If the deadline has expired at the start of the request, `redis::RequestException` is thrown
   with `GetStatus() == redis::ReplyStatus::kTimeoutError`
2. The request timeout is updated from the deadline
    * If the request is delayed after that, then the usual redis timeout exception is thrown: `redis::RequestException`
      with `GetStatus() == redis::ReplyStatus::kTimeoutError`


----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/os_signals.md | @ref scripts/docs/en/userver/caches.md ⇨
@htmlonly </div> @endhtmlonly
