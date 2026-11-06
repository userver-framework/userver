# Statistics and Metrics

## What are Metrics?

Metrics are numerical values representing specific indicators that change over time (time series). They can be collected
from service via special agents and sent to a metrics storage system (Prometheus/Graphite/VictoriaMetrics/...). As
numerical values, metrics typically capture fundamental service indicators that signal the overall health of a service
or subsystem (e.g., the background error rate in a message queue handler). Services log additional detailed textual
information for troubleshooting in logs. Logs help identify the root causes of errors, correlate them with client
issues, trace causality, etc. Attempting to encode textual event information into metric names usually results in
metrics bloat and makes them difficult to navigate.

There guarantees regarding the delivery of metrics mostly depend on metrics delivery agent and metrics storage system.
Some metrics may fail to reach the server, and already saved metrics may become temporarily unavailable due
to server downtime (e.g., for several hours), metrics storage downtime or special agent malfunction.

If higher guarantees of availability and reliability for delivery and access are required, it's recommended to use
another storage system or agent that provide such guarantees. Specifically, if metrics are involved in computations
critical to service functionality (e.g., triggering fallbacks), you should use a database with stronger guarantees
than metrics.

The metrics collection interval depend on the agent, userver may report metrics at any rate.


## What to Measure?

Services, daemons, cron tasks, and other programs perform useful work. Characteristics of this work should be reflected
in metrics. There are four main categories of informative metrics:

1.  **Successfully processed requests/messages/tasks.** Basic metrics showing the volume of work the service performs.
2.  **Errors.** Critical metrics indicating the presence or absence of explicit problems in the service. It's usually
    valuable to track both the total number of errors and errors by type (e.g., client errors, server errors, timeouts,
    socket errors, task cancellations, etc.).
3.  **Performance speed/Latency.** For handlers, measure request/message processing times (timings). Since processing
    time varies per message/request, timing aggregates are typically tracked as percentiles. For message/task
    queues, queue length is also measured (in the number of messages or the time a message waits to be processed).
4.  **Resource utilization.** This applies to both physical and virtual resources. A service can monitor the usage of
    existing resources (e.g., OS resources like CPU, RAM, network) or create its own (e.g., connection pool or worker
    pool utilization). These metrics are essential for determining resource efficiency and predicting when current
    resources will become insufficient.

In specific cases, more exotic metrics might also be appropriate.

When introducing new metrics, keep in mind that storing metrics is not free and consumes space in metrics storage.
Only implement metrics that genuinely reflect the service's state and can help developers or operations understand the
health of the service or feature.


## Existing Metrics and Statistics of userver

Components from userver provide metrics by default. No additional setup beyond including the component is required.

See @ref scripts/docs/en/userver/service_monitor.md for information on how to retrieve metrics and for a list of
metrics.


## How to write metrics and statistics

### How NOT to Write Metrics

1. **Callback Function Should Not Modify Service State.** The statistics handler might be triggered not only for sending
   metrics to metrics storage but also for other purposes; a cron task might unexpectedly poll the service more
   frequently or a manual request for metrics can be performed. Therefore, the approach
   "read the current counter value, emit it as a metric, then reset the counter" is invalid. Instead, use a monotonic
   counter. **Resets are permitted only in tests**. Only optimizations that do not affect the final result
   (e.g., caching or memoization) are allowed.

2. **Callback Function Should Not Access External Services.** All data required for metric generation must reside in the
   process's memory. For metrics requiring DB access, standard practice involves caching database responses in a
   background variable and computing metrics from this cached value.

3. **Avoid Blocking Synchronization Primitives.** The callback should ideally avoid synchronization primitives that may
   cause blocking (mutexes, semaphores, etc.). Your service should be able to report metrics even if critical
   components are deadlocked.

4. **Minimize Observation Overhead.** When adding metric collection, ensure it does not slow down the observed code.
   Event tracking must be maximally efficient.

5. **Optimize for Event Accounting Over Metric Reporting.** While the speed of the statistics endpoint is less critical,
   prefer faster event accounting even if it slows down final metric aggregation. Favor this tradeoff.


### How to Register Your Metric Set

**Method 1**: Via @ref utils::statistics::MetricTag

Register a metric type via @ref utils::statistics::MetricTag and provide a `DumpMetric` function for exporting it:

@snippet samples/tcp_full_duplex_service/main.cpp  TCP sample - Stats tag

Now the tag could be used in component constructor to get a reference to the
`struct Stats`:

@snippet samples/tcp_full_duplex_service/main.cpp  TCP sample - constructor

See also @ref scripts/docs/en/userver/tutorial/tcp_full.md for a complete example.


**Method 2**: Register a writer callback

In a component constructor, use @ref utils::statistics::RegisterWriterScope with the component context. The helper
retrieves the default @ref components::StatisticsStorage component and registers the callback with its
@ref utils::statistics::Storage.

For a component exporting its own state, a `WriteStatistics` member called by the callback is sufficient. The component
does not need its own `DumpMetric` just to export its state. Structures that group its metrics can provide `DumpMetric`,
so they can be written from `WriteStatistics` and tested independently of the component.

@snippet core/src/clients/dns/component.cpp RegisterWriterScope component sample

In a helper class that is not a component, accept a @ref utils::ResourceScopeStorage reference as the first
constructor argument and a @ref utils::statistics::Storage reference from the owner. Register the callback directly with
@ref utils::statistics::Storage::RegisterWriter, passing the scopes as the first argument:

@snippet core/src/utils/statistics/storage_test.cpp  Writer basic sample

See also @ref utils::statistics::Writer for more details.


#### Registration lifetime

Register from the constructor. Registration follows the lifetime rules of @ref utils::ResourceScopeStorage.
For callback requirements, see @ref utils::statistics::RegisterWriterScope and
@ref utils::statistics::Storage::RegisterWriter.

For helper objects, choose scopes according to when the helper is constructed and destroyed:

- A helper created during component construction and owned by that component until its destruction can receive
  `context.Scopes()` as its first constructor argument. One example is an `Impl` object holding the component's
  implementation. Another is a @ref clients_from_components_lifetime "client" owned by the component and exposed to
  other components by reference.
- A helper created after component construction or destroyed before the component needs its own scopes. Use
  @ref utils::WithResourceScopes, or @ref utils::MakeWithResourceScopes when shared ownership is needed. For example,
  this applies to objects that a factory component creates while the service is running.

Both wrappers pass a @ref utils::ResourceScopeStorage reference as the first constructor argument. They register the
callback after the helper is fully constructed and unregister it before its destructor starts.

Passing a factory component's scopes to the helpers it creates ties their registrations to the component's lifetime.
If a helper is destroyed earlier, its callback remains registered and can access the destroyed helper.

Do not store @ref utils::ResourceScopeStorage as a member of the helper itself: the helper's destructor has already
started when its members are destroyed. Use the wrapper to unregister the callback before calling that destructor.


### Metric Types

Built-in metric types and user-defined types with `DumpMetric` can be written directly to a
@ref utils::statistics::Writer.


#### Basic Primitives for Metrics
- @ref utils::statistics::RateCounter - For `RATE` metrics that calculate RPS (a monotonic counter whose derivative is
  automatically computed).
  - @ref utils::statistics::RateCounter : An atomic counter (similar to `std::atomic<Rate>`).
  - @ref utils::statistics::Rate : A non-atomic metric value. Used to aggregate multiple
    @ref utils::statistics::RateCounter instances.
  - Dashboard tip: Do **not** apply `non_negative_derivative` to these. Use integral to calculate totals over a period.

- `std::atomic<std::uint64_t>`
  For `GAUGE` metrics (current value of a quantity).
  Best practice: Avoid dedicated variables when the live value can be read directly in the registered writer callback.
  Typical use cases:
  - Current indicator value (e.g., queue length saved in a variable during a periodic task).
  - Complex aggregates (e.g., total message body lengths used to compute average message length over a
    period: (delta sizes / delta count)).
  Use @ref utils::statistics::RateCounter for monotonic event counts. Replacing a gauge with a rate counter changes
  the exported metric type and how monitoring systems interpret it.

- @ref utils::statistics::RelaxedCounter - An alternative to `std::atomic` for `GAUGE` metrics, using
  `std::memory_order_relaxed`. Use @ref utils::statistics::RateCounter for monotonic event counts exported as `RATE`.

- @ref utils::statistics::Histogram and @ref utils::statistics::HistogramAggregator
  For `HIST_RATE` metrics (statistics for events, e.g., timings).
  - @ref utils::statistics::Histogram : an atomic counter.
  - @ref utils::statistics::HistogramAggregator : Non-atomic metric value. Aggregates multiple
    @ref utils::statistics::Histogram.
  - Bucket configuration required (max 50 buckets, per monitoring docs). Exponential buckets can achieve ~10% error
    for timings.
  @warning Creating histograms consumes `~50` metrics each. Consider the quota impact of your metrics storage!


#### Additional Primitives
- @ref utils::statistics::RecentPeriod - Template class for metric values "over the last N seconds".
  - Works with `GAUGE` counters. Incompatible with @ref utils::statistics::RateCounter and
    @ref utils::statistics::Histogram.
  - Drawback: Not aggregatable across hosts (summing host values yields meaningless results for timings).
    Avoid in new code; use @ref utils::statistics::Histogram or @ref utils::statistics::MinMaxAvg

- @ref utils::statistics::Percentile - Computes response timings for the k-th percentile, should be used with
  @ref utils::statistics::RecentPeriod.
  - Cons: Not aggregatable across hosts. Prefer @ref utils::statistics::Histogram.

- @ref utils::statistics::MinMaxAvg - Tracks minimum, maximum, and average values.
  - Inherits all @ref utils::statistics::RecentPeriod drawbacks.


#### Custom User Metric Types

Instead of writing one giant `DumpMetric` or `WriteStatistics` somewhere, it is recommended to break these functions
into pieces:

@snippet universal/src/utils/statistics/writer_test.cpp  DumpMetric basic

DumpMetric functions nest well, labels are propagated to the nested DumpMetric:

@snippet universal/src/utils/statistics/writer_test.cpp  DumpMetric nested

Recommendations for organizing custom metric types:

* Define a `DumpMetric` for each custom type that groups metrics.
* Compose the serialization of complex types from the serialization of nested types.
* Write objects through @ref utils::statistics::Writer::operator= or
  @ref utils::statistics::Writer::ValueWithLabels rather than calling `DumpMetric` explicitly.

Declare `DumpMetric` alongside the type, in the same namespace, so argument-dependent lookup (ADL) can find it.
Define it in the corresponding `.cpp`; template and header-only types may define it in the header. A declaration
that only mentions @ref utils::statistics::Writer can include `<userver/utils/statistics/fwd.hpp>`; code using the
writer's operations needs `<userver/utils/statistics/writer.hpp>`.

Keep the representation with the type: write a nested object as `writer["child"] = stats.child` rather than repeating
its field-by-field export at every call site.

Put loops over containers inside `DumpMetric` for your own enclosing type. Do not add overloads for third-party
containers or aliases of those containers.

If the same data needs distinct representations, use wrapper types with their own `DumpMetric` functions.

Provide `ResetMetric` if the type supports resetting. It is not required for exporting metrics. For tests, prefer
@ref pytest_userver.client.ClientMonitor.metrics_diff to resetting shared service state.

Generic helpers can constrain metric arguments with @ref utils::statistics::HasWriterSupport.


### Using a writer

#### Writing values

Write supported metric objects directly. Calling `.load()` or `.Load()` just to export a counter is unnecessary.
Explicit reads are appropriate when computing a value, such as an error ratio.

| Metric type | Writing its value |
| --- | --- |
| @ref utils::statistics::RateCounter | `writer["requests"] = requests` |
| `std::atomic<std::uint64_t>` | `writer["active"] = active` |
| @ref utils::statistics::RelaxedCounter | `writer["active"] = active` |

Plain integers, `double`, and @ref utils::statistics::Rate can also be written directly. Integers and `double` are
exported as `GAUGE`; `Rate` is exported as `RATE`. This is useful for computed values and for data protected by a mutex.
Read protected data under the same mutex when exporting it. See
@ref metrics_database_collection "Database metrics" for background collection.

Other metric types from @ref utils::statistics and user-defined types with `DumpMetric` are supported too.
Define `void DumpMetric(utils::statistics::Writer&, const T&)` in the namespace of the user-defined type `T`.

Writing `writer["timings"] = timings`, where `timings` is a @ref utils::statistics::RecentPeriod, aggregates the
configured window without the current incomplete epoch. There is no need to call `timings.GetStatsForPeriod()` just
for this assignment. To select another interval or include the current epoch, call
@ref utils::statistics::RecentPeriod::GetStatsForPeriod with the desired arguments and write its result.

Write each metric, identified by its path and labels, once per collection. Repeated writes do not replace the previous
value. Compute the final value before exporting it; see @ref utils::statistics::Writer for the contract.


#### Paths

Use a local child @ref utils::statistics::Writer when several metrics share a path prefix, and write only the suffixes
through it. A single metric usually needs no local child. The registration example above groups `mc4` and `mc5`
under the same `a.b` prefix.

See @ref utils::statistics::Writer for child lifetime restrictions and
@ref utils::statistics::Writer::operator[] for path semantics. In particular, `writer["a.b"]` and `writer["a"]["b"]`
produce the same path; output formats handle escaping.

Avoid creating temporary `std::string` copies solely to pass a path. Path forwarding helpers can take
`std::string_view`.


#### Labels

Attach labels shared by all fields once with @ref utils::statistics::Writer::ValueWithLabels on the whole object.
The object's `DumpMetric` propagates them to nested metrics. Labels that vary between metrics belong on those individual
metrics. See the nested `DumpMetric` example in the custom metric types section above.

Label forwarding helpers can take `std::string_view` too. Keep owning strings when storing data.
See @ref utils::statistics::Writer::ValueWithLabels for label propagation and string lifetime requirements.


### Database Metrics

@anchor metrics_database_collection

It's common to need to expose database data in metrics. Querying the database directly from the metrics handler is
discouraged, as metrics must be returned quickly and should not depend on external resource states
(e.g., DB availability). Instead, use a @ref utils::PeriodicTask or DistLock to query the database and cache results
in process memory. See also @ref scripts/docs/en/userver/periodics.md.

For current state computed by a periodic task, use `GAUGE` metrics stored in atomics or a @ref concurrent::Variable.
With `concurrent::Variable`, the task updates the data and the metrics callback reads it under the same lock. Perform
expensive work before acquiring the lock, then publish the result in a short critical section. Count events, such as
successful task runs, separately with @ref utils::statistics::RateCounter.

Key scenarios where such metrics are useful:
1. Object count in a table:
   - For a database-backed message queue, this, reflects queue size.
   - For rough estimates (to reduce DB load), use reltuples instead of `COUNT`.
2. Objects matching specific queries:
   - Helps evaluate "lost," "unprocessed," or otherwise problematic records.

⚠️ Critical before production:
- Always analyze query performance and index usage via `EXPLAIN ANALYZE`.
- Metric collection must not significantly strain the database.


## How to test metrics in testsuite and gtest

Prefer gtest when the metric logic is local to a component; use testsuite when the assertion needs a running service and
the monitor HTTP endpoint.

### gtest

For metrics declared through @ref utils::statistics::MetricTag, create a @ref utils::statistics::MetricsStorage
in the test:

@snippet core/src/utils/statistics/metrics_storage_test.cpp unit testing metrics

To unit test a metric type with `DumpMetric`, pass an object directly to @ref utils::statistics::Snapshot:

@snippet universal/utest/src/utils/statistics/testing_test.cpp  metrics Snapshot DumpMetric sample

Test the production `DumpMetric` directly; do not reproduce its export logic in a test-only function. There is no need
to create a @ref utils::statistics::Storage and register a writer solely to inspect an object that already supports
`DumpMetric`.

If the class registers writers in @ref utils::statistics::Storage, pass Storage:

@snippet core/src/utils/statistics/storage_test.cpp  metrics Snapshot sample

When the same logical metric is split across paths or label sets, dump once and compare subgroups:

@snippet universal/utest/src/utils/statistics/testing_test.cpp metrics Snapshot subgroup sample

### testsuite

Use @ref pytest_userver.plugins.service_client.monitor_client "monitor_client" to check metrics from a running service.
See @ref scripts/docs/en/userver/testsuite_metrics_testing.md for the API, examples, snapshot comparisons, and
before/after assertions with `metrics_diff()`.

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/requests_in_flight.md | @ref scripts/docs/en/userver/service_monitor.md ⇨
@htmlonly </div> @endhtmlonly

