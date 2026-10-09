# Experimental MongoDB driver integration

**This implementation is experimental.** It is an alternative to the legacy
userver integration in `../cdriver`, with the same public `storages::mongo` API.

**As of October 5, 2026, no released upstream mongo-c-driver version provides
all the changes required to use this implementation.** A patched driver is
required. The current integration targets mongo-c-driver 1.30.10 with backports
of the public changes listed below; installing an unmodified system driver is
not sufficient.

## Goals

- Unify client management with the native `mongoc_client_pool_t`, reducing the
  amount of pool logic maintained separately in userver.
- Reduce the overhead of creating new connections by acquiring lazy client
  objects without a separate warm-up `ping`, and by sharing topology monitoring.
- React to MongoDB cluster topology changes faster and more smoothly through
  the native pool's shared topology and background monitoring.

These are design goals, not guarantees of lower latency in every workload.
Actual network connections still require DNS resolution, TCP connection setup,
TLS when enabled, and MongoDB handshake/authentication.

## Required mongo-c-driver changes

The required public changes are listed below. PR status and commit references
were checked on October 5, 2026:

| Change | Public PR or commit | Purpose |
| --- | --- | --- |
| Base64 initialization through the common once abstraction | [Merged commit `67a62bf55256fcd17738822fd1c2a2c2472d4369`](https://github.com/mongodb/mongo-c-driver/commit/67a62bf55256fcd17738822fd1c2a2c2472d4369) | Route initialization through the threading abstraction. Only this part of the upstream commit is backported. |
| Wake pool waiters when the maximum size increases | [PR #2445](https://github.com/mongodb/mongo-c-driver/pull/2445) (merged), [merged commit `a04f73b7486ce02d57e5e03aeac3e996cb741b1d`](https://github.com/mongodb/mongo-c-driver/commit/a04f73b7486ce02d57e5e03aeac3e996cb741b1d) | Correct native blocking `pop` behavior when capacity grows. The userver acquisition path itself uses `try_pop`. |
| Pool stream initiator | [PR #2444](https://github.com/mongodb/mongo-c-driver/pull/2444) (closed without merging), [proposed commit `d5353f553ddf3f1847ba4c1adea8ba12296c26a2`](https://github.com/mongodb/mongo-c-driver/commit/d5353f553ddf3f1847ba4c1adea8ba12296c26a2) | Install userver's asynchronous transport for both application clients and topology monitoring. |
| Enforce a reduced maximum size and allow destruction with a zero limit | [PR #2447](https://github.com/mongodb/mongo-c-driver/pull/2447) (open), [current commit `95d524bebd3f712e6224bae7790b44f70bc4b645`](https://github.com/mongodb/mongo-c-driver/commit/95d524bebd3f712e6224bae7790b44f70bc4b645) | Remove idle clients immediately on a limit reduction, retire checked-out clients on return, and avoid blocking `endSessions` cleanup in a drained pool. |
| Runtime thread backend | [PR #2448](https://github.com/mongodb/mongo-c-driver/pull/2448) (closed without merging), [proposed commit `a2f71dbee525031c7db4e8ad65b53bf651c5c078`](https://github.com/mongodb/mongo-c-driver/commit/a2f71dbee525031c7db4e8ad65b53bf651c5c078) | Adapt the driver's threads, synchronization, once initialization, and sleep operations to userver, with a size-aware backend table. |
| Client pool statistics | [PR #2454](https://github.com/mongodb/mongo-c-driver/pull/2454) (open), [current commit `050ed442ac5ed72b876b4a6ab9beea5461568884`](https://github.com/mongodb/mongo-c-driver/commit/050ed442ac5ed72b876b4a6ab9beea5461568884) | Read a consistent snapshot of created, destroyed, and currently owned client objects, including checked-out clients. The 1.30.10 backport also counts removals in its idle policy and the shrinking helper from PR #2447. |
| Concurrent connection establishment | [PR #2458](https://github.com/mongodb/mongo-c-driver/pull/2458) (open), [current commit `af08d8548fd5ff4296ed398739ba3e0018267df5`](https://github.com/mongodb/mongo-c-driver/commit/af08d8548fd5ff4296ed398739ba3e0018267df5) | Implement `maxConnecting` per server, with bounded slot waiting and wakeups for all eligible servers. The 1.30.10 backport uses its monotonic clock instead of the newer mlib timer. |

PRs #2444 and #2448 were closed without merging on October 1, 2026. In the
[upstream discussion](https://github.com/mongodb/mongo-c-driver/pull/2444),
maintainers explained that suspending threads before blocking is an unsupported
use case and suggested maintaining the necessary changes as local patches.
Their proposed commits remain public and are still required by this integration.
PRs #2447, #2454, and #2458 remain open as independent pool improvements.

The public changes target upstream code and require adaptation when backported
to 1.30.10. All of the changes above are required for this integration, including
those from closed PRs.

## Implementation and behavior

The legacy pool manages individually created `mongoc_client_t` objects in
userver. Each client has its own single-threaded topology. The experimental
implementation delegates client storage and lifecycle to
`mongoc_client_pool_t`: clients in a pool generation share topology state and
background monitoring. Driver operations use userver's transport and runtime
thread backend so that network waits and synchronization can cooperate with
the coroutine scheduler.

| Behavior | Legacy integration | Experimental integration |
| --- | --- | --- |
| Client acquisition | Userver manages idle clients and creates clients with `mongoc_client_new_from_uri`. Creation includes a `ping` using `nearest`. | A userver cancellable semaphore controls admission, then native `mongoc_client_pool_try_pop` returns or creates a lazy client. Acquisition does not perform a warm-up `ping`. |
| Connection readiness | The creation `ping` establishes connectivity to a selected server, but does not guarantee a connection to the server chosen by a later operation with different read preferences. | A returned client does not imply an established connection. Server selection and connection setup happen when needed by an operation. Explicit `Pool::Ping()` remains available. |
| Topology monitoring | Clients maintain their own topology and perform single-threaded discovery/selection. | Clients share the native pool's topology and background monitors, including their transport configuration. |
| `initial_size` | Prepopulates clients with a connectivity check. | Prepopulates lazy client objects without a connectivity check; successful pool initialization does not imply that MongoDB is reachable. |
| `connecting_limit` | Limits concurrent client creation, including the creation `ping`. | Defaults the URI option `maxConnecting`, limiting concurrent application connection setup per server through handshake and authentication. An explicit URI option takes precedence. Lazy client-object creation and monitoring connections are not limited. |
| `idle_limit` | A target for periodic idle-client trimming. | Maps to 1.30.10's native `minPoolSize` policy: at most one excess idle client is removed per return; zero disables this trimming. |
| `maintenance_period` | Controls periodic idle maintenance. | Not applied; native idle trimming happens on return. |
| Maximum size reduction | Enforced by the userver pool's capacity and lifecycle logic. | Updates both the admission semaphore and native pool limit. The patched native pool removes excess idle clients and retires excess busy clients when returned. |
| URI replacement | Subsequent acquisitions use the updated URI; clients from the previous URI are retired. | Creates a new native pool generation. The old generation's maximum is reduced to one, preserving a client for best-effort session cleanup; checked-out clients retain the generation until return, including for ongoing transactions. |

Both implementations support queue timeouts, cancellation and inherited
deadlines during acquisition, dynamic pool settings, and updating the default
database name after a URI change.

Generation destruction uses the native pool's best-effort `endSessions` before
closing its remaining clients. Retired generations retain at most one client
after all borrowed clients return, allowing cleanup to reuse an existing connection.
Cleanup uses the configured server-selection and socket timeouts and does not
establish a new connection solely for `endSessions`. If all clients were already
removed (for example, by explicitly setting a zero maximum), cleanup is skipped.
The final client statistics are accounted after native pool destruction.

When the last client releases a retired generation, the pool wrapper schedules
its destruction in a background task, so stopping topology monitors does not delay
the client's return or hold its admission slot. The wrapper waits for scheduled
cleanup tasks on shutdown. Once shutdown stops accepting cleanup tasks, remaining
clients or cursors destroy their generations synchronously when released; this
also supports cursors that outlive the wrapper.

Changing `connecting_limit` creates a new pool generation only when it changes
the effective `maxConnecting`; clients already acquired keep the previous generation.
The latest default also applies after URI replacement. Waiting for a connection slot
uses a positive URI `waitQueueTimeoutMS`, or otherwise `connectTimeoutMS`. This is
a separate budget from connection establishment and does not inherit the operation deadline.
Exhausting this budget reports a local queue timeout; it does not mark the server
unavailable or invalidate its existing connections.

Dynamic configuration retains the existing minimum of 1 for `idle_limit` in both
implementations. The experimental native policy's zero value is available through
static pool settings, provided `initial_size` is also zero.

The idle policy above is specific to the patched 1.30.10 implementation.
The deprecated `minPoolSize` API was removed in upstream 2.x, so upgrading the
driver requires revisiting that policy rather than applying the patches blindly.

## Metrics

The experimental pool's connection metrics count native client objects, not
physical TCP connections. A client may connect to several servers, and
background topology monitoring uses additional connections.

| Metric | Experimental pool semantics |
| --- | --- |
| `mongo.pool.current-size` | Idle and borrowed client objects across all live generations, including generations draining after URI or `connecting_limit` changes. It may temporarily exceed the active generation's maximum. |
| `mongo.pool.max-size` | Current limit on simultaneously borrowed clients, enforced by the admission semaphore and active native pool. |
| `mongo.pool.current-in-use` | Occupied admission permits, including clients being acquired and clients borrowed from previous generations. It may temporarily exceed `max-size` after a reduction, or `current-size` during client creation. |
| `mongo.pool.conn-created`, `mongo.pool.conn-closed` | Cumulative created and destroyed client-object counters. They survive generation changes and include draining generations. |

The wrapper adds differences between serialized statistics snapshots of each
generation. Repeated reads do not count the same events twice. Snapshots are
refreshed after acquisition, return, and limit changes, with a final snapshot
before destruction.

Cursor success metrics count batch fetches rather than documents read from a
local batch. Heartbeat and topology metrics aggregate all live generations.

Collections, cursors, database commands, request helpers, and transactions use
the same implementation for both pools. `cdriver/pool_access.hpp` adapts client
ownership, cursor statistics, and generation-specific bulk-write support without
allocating an additional client wrapper on the heap.

With the runtime backend enabled, both pools share the socket, TLS, and buffered
I/O implementation in `cdriver/async_stream.cpp`. Each stream selects its polling implementation at
creation: the legacy pool reuses a task-local poller, while the native pool
uses a poller per call and accounts for buffered input.

The experimental transport does not use the legacy pool's TCP host blocking
and background TCP probes. Host availability and reconnection are managed by
the native driver's topology monitoring. Connection successes and failures in
the experimental transport do not update the legacy host failure counters.

## Selecting the implementation

With a patched mongo-c-driver, both the experimental pool and the process-wide
runtime backend are enabled by default. They can also be selected explicitly
on a `Mongo` or `MultiMongo` component:

```yaml
components_manager:
    userver_experiments:
        mongo-thread-backend: true
    components:
        mongo-foo:
            dbalias: foo
            driver: mongo-c-driver-experimental
```

Set `driver: mongo-c-driver` explicitly to select the legacy integration.
Builds without the required patched-driver APIs use the legacy pool by default.
The selection is fixed for the pool's lifetime; changing it requires a restart.
Legacy and experimental pools can coexist in the same process.

CMake checks for the required runtime-backend, stream-initiator, and pool-statistics
APIs and the `maxConnecting` option declaration. Without them, experimental sources
and tests are excluded, and enabling the static option is rejected during configuration
validation. These API checks do not replace applying the pool-behavior fixes in the
patch series.

The `mongo-thread-backend` experiment is enabled by default and can be disabled
with `components_manager.userver_experiments.mongo-thread-backend: false`.
With it disabled, both pool implementations use native threading primitives.
The experimental pool logs a warning and uses the driver's native blocking
transport, including for topology monitors. Builds without the backend API log
a warning and continue using the legacy pool with native threading primitives.
These defaults also apply to pools constructed directly through the C++ API.

The backend is selected once, on a native stack during task-processor thread
startup, before `mongoc_init` and before tasks execute. Changing the setting
requires a process restart; attempts to create pools with a setting that differs
from the initialized mode are rejected. Programs using `engine::RunStandalone`
must select the experiment before creating their first task processor.

When enabled, the backend is shared by both pool implementations and all other
libmongoc/libbson users in the process. Regular driver operations that use it
must run in userver coroutines. Mutex access from ordinary native threads is
only supported during the integration's global initialization and cleanup
scopes; creating or joining driver threads requires a userver coroutine.

The runtime backend table sets `struct_size` to `sizeof(bson_thread_backend_t)`.
The driver copies only the provided part of the table, with native defaults for
omitted fields, so the table can be extended without reading beyond a caller's
older structure. Invalid sizes are rejected without replacing the backend.

## Runtime backend use with the legacy pool

With `mongo-thread-backend: true`, the legacy client uses the installed runtime
backend even though its topology is single-threaded. This was observed by tracing
backend callbacks while running the legacy parametrization of the MongoDB tests on October 5, 2026: 106 tests
passed and 5 were skipped across the MongoDB 8 and MongoDB 5 test suites. The
strict `*/Legacy` filter excludes tests that instantiate both implementations.
Tracing deduplicated stacks by their first five frames and recorded at most 64
distinct stacks per callback and execution context in each test process.

Observed coroutine call paths include:

| Backend callback | Driver paths observed |
| --- | --- |
| `mutex_lock` / `mutex_unlock` | Handshake metadata freeze and scanner command setup; topology-description modification while applying `$clusterTime`; server-session pool access; libmongoc logging. |
| `shared_mutex_lock_shared` / `shared_mutex_unlock_shared` | Loading the shared topology description during server selection, stream selection, application error handling, and session operations. |
| `shared_mutex_lock` / `shared_mutex_unlock` | Publishing a changed shared topology description. |
| `once` | Lazy initialization for shared-pointer synchronization, libmongoc logging, and the default BSON context used by ObjectId creation. |
| `mutex_init` / `mutex_destroy`, `cond_init` / `cond_destroy` | Constructing and destroying each client's topology, including its session pool and monitor/logging state. |

On native stacks, global `mongoc_init` / `mongoc_cleanup` also use `once` and
mutex callbacks for logging, handshake data, AWS credentials, and the OCSP cache.
The legacy client's topology creation goes through
`mongoc_client_new_from_uri` to `mongoc_topology_new`. That function
unconditionally creates `topology->session_pool` with
`mongoc_server_session_pool_new_with_params`; the `MONGOC_DECL_SPECIAL_TS_POOL`
wrapper implements that type over `mongoc_ts_pool`. The pool's mutex protects
its reusable server-session list in `_try_get`, `mongoc_ts_pool_return`, and
related operations. This construction is not conditional on
`topology->single_threaded`. Single-threaded topology disables background
monitoring threads; it does not remove synchronization from topology state,
logging, or server-session management.

No thread create/join, condition-variable wait/notification, backend sleep,
`mutex_is_locked`, or shared-mutex destruction callback was observed in this
test run. Legacy server-selection sleep uses the separate callback installed by
`mongoc_client_set_usleep_impl`, which calls userver's `engine::SleepFor`.
These results describe exercised test paths only: they do not establish callback
frequency, lock contention, or whether every production configuration uses the
same paths. Authentication- and TLS-specific paths were not independently
covered.
