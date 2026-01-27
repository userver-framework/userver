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

To automatically deliver the metrics they should be registered via
@ref utils::statistics::MetricTag and `DumpMetric` + `ResetMetric` functions should be
defined:

@snippet samples/tcp_full_duplex_service/main.cpp  TCP sample - Stats tag

Now the tag could be used in component constructor to get a reference to the
`struct Stats`:

@snippet samples/tcp_full_duplex_service/main.cpp  TCP sample - constructor

Note that using `reset_metrics()` is discouraged, prefer using a more reliable
@ref pytest_userver.client.ClientMonitor.metrics_diff "await monitor_client.metrics_diff()".

See also @ref scripts/docs/en/userver/tutorial/tcp_full.md for a complete example.


**Method 2**: Via @ref utils::statistics::Writer

Retrieve the @ref utils::statistics::Storage component and register a writer for your component via
@ref utils::statistics::Storage::RegisterWriter:

@snippet core/src/utils/statistics/pretty_format_test.cpp  Writer basic sample

See also @ref utils::statistics::Writer for more details.


### Metric Types

A metric type is any class for which `DumpMetric` is defined. Such types can be written directly to a
@ref utils::statistics::Writer.


#### Basic Primitives for Metrics
- @ref utils::statistics::RateCounter - For `RATE` metrics that calculate RPS (a monotonic counter whose derivative is
  automatically computed).
  - @ref utils::statistics::RateCounter: An atomic counter (similar to `std::atomic<Rate>`).
  - @ref utils::statistics::Rate: A non-atomic metric value. Used to aggregate multiple
    @ref utils::statistics::RateCounter instances.
  - Dashboard tip: Do **not** apply `non_negative_derivative` to these. Use integral to calculate totals over a period.

- `std::atomic<std::uint64_t>`
  For `GAUGE` metrics (current value of a quantity).
  Best practice: Avoid dedicated variables; instead, write live values via RegisterWriter.
  Typical use cases:
  - Current indicator value (e.g., queue length saved in a variable during a periodic task).
  - Complex aggregates (e.g., total message body lengths used to compute average message length over a
    period: (delta sizes / delta count)).
  @warning Deprecated. Use @ref utils::statistics::RateCounter instead

- @ref utils::statistics::Histogram and @ref utils::statistics::HistogramAggregator
  For `HIST_RATE` metrics (statistics for events, e.g., timings).
  - @ref utils::statistics::Histogram: an atomic counter.
  - @ref utils::statistics::HistogramAggregator: Non-atomic metric value. Aggregates multiple
    @ref utils::statistics::Histograms.
  - Bucket configuration required (max 50 buckets, per monitoring docs). Exponential buckets can achieve ~10% error
    for timings.
  @warning Creating histograms consumes `~50` metrics each. Consider the quota impact of your metrics storage!


#### Additional Primitives
- @ref utils::statistics::RecentPeriod - Template class for metric values "over the last N seconds".
  - Works with `GAUGE` counters. Incompatible with @ref utils::statistics::RateCounter and
    @ref utils::statistics::Histogram.
  - Drawback: Not aggregatable across hosts (summing host values yields meaningless results for timings).
    Avoid in new code; use @utils::statistics::Histogram or @utils::statistics::MinMaxAvg

- @ref utils::statistics::Percentile - Computes response timings for the k-th percentile, should be used with
  @ref utils::statistics::RecentPeriod.
  - Cons: Not aggregatable across hosts. Prefer @ref utils::statistics::Histogram.

- @ref utils::statistics::MinMaxAvg - Tracks minimum, maximum, and average values.
  - Inherits all @ref utils::statistics::RecentPeriod drawbacks.


#### Custom User Metric Types

Instead of writing one giant `DumpMetric` or `WriteStatistics` somewhere, it is recommended to break these functions
into pieces:

@snippet core/src/utils/statistics/writer_test.cpp  DumpMetric basic

DumpMetric functions nest well, labels are propagated to the nested DumpMetric:

@snippet core/src/utils/statistics/writer_test.cpp  DumpMetric nested

**Recommendation:**

* Define a `DumpMetric` for each metric type.
* Compose the serialization of "large" metric types from smaller ones, like blocks.
* Instead of calling `DumpMetric` directly, call utils::statistics::Writer::operator[] or
  utils::statistics::Writer::ValueWithLabels.


### Database Metrics

It's common to need to expose database data in metrics. Querying the database directly from the metrics handler is
discouraged, as metrics must be returned quickly and should not depend on external resource states
(e.g., DB availability). Instead, use a @ref utils::PeriodicTask or DistLock to query the database and cache results
in atomic variables in process memory. See also @ref scripts/docs/en/userver/periodics.md

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

@ref utils::statistics::MetricsStorage is easy to create in unit tests (gtest). If you need to test classes that use
@ref utils::statistics::Storage, you can use @ref utils::statistics::Snapshot.

Prefer testing metrics in unit tests if possible. Otherwise, metrics could be tested in testsuite, see
@ref TESTSUITE_METRICS_TESTING "Metrics testing in testsuite".

**Short Examples for the Impatient**:

- Retrieve specific service metric by path and (optionally) labels:
  @snippet samples/testsuite-support/tests/test_metrics.py metrics single_metric

- Retrieve array of metrics by path prefix and (optionally) labels:
  @snippet samples/testsuite-support/tests/test_metrics.py metrics metrics

- Retrieve specific service metric by path and (optionally) labels or `None` if no such metric:
  @snippet samples/testsuite-support/tests/test_metrics.py metrics single_metric_optional

- Diff of metrics:
  @snippet samples/testsuite-support/tests/test_metrics.py metrics diff

- Reset metrics (discouraged):
  @snippet samples/testsuite-support/tests/test_metrics.py metrics reset

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/requests_in_flight.md | @ref scripts/docs/en/userver/service_monitor.md ⇨
@htmlonly </div> @endhtmlonly

