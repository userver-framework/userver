# Metrics testing in testsuite

@anchor TESTSUITE_METRICS_TESTING

Testsuite exposes userver metrics written via @ref utils::statistics::Writer and
@ref utils::statistics::MetricTag through
@ref pytest_userver.plugins.service_client.monitor_client "monitor_client".
Configure the monitor listener as in @ref tutorial_metrics "the metrics tutorial" and
@ref scripts/docs/en/userver/service_monitor.md.

For gtest and @ref utils::statistics::Snapshot, see
@ref scripts/docs/en/userver/metrics.md "Statistics and Metrics — How to test metrics in testsuite and gtest".

The API docstrings are the source of truth: @ref pytest_userver.client.ClientMonitor and
@ref pytest_userver.metrics.MetricsSnapshot.


## Monitor client API

- Retrieve one metric (must exist):
  @ref pytest_userver.client.ClientMonitor.single_metric "await monitor_client.single_metric(path, labels=labels)"
- Retrieve multiple metrics by path prefix:
  @ref pytest_userver.client.ClientMonitor.metrics "await monitor_client.metrics(prefix=prefix, labels=labels)"
- Retrieve one metric or `None`:
  @ref pytest_userver.client.ClientMonitor.single_metric_optional "await monitor_client.single_metric_optional(path, labels=labels)"
- Before/after diff:
  @ref pytest_userver.client.ClientMonitor.metrics_diff "async with monitor_client.metrics_diff() ..."
- Reset metrics (discouraged):
  @ref pytest_userver.client.Client.reset_metrics "await service_client.reset_metrics()"


## Choose the right API

Choose the method according to what the test checks:

1. **Complete ground-truth comparison.** Fetch once with `metrics(prefix=...)` and compare to
   `MetricsSnapshot.from_layered_dict(...)` or a constructed `MetricsSnapshot`.
2. **One metric.** Use `single_metric()` when the metric must exist, or `single_metric_optional()` when absence
   is expected.
3. **Several values from one subtree.** Fetch once with `metrics(prefix=...)`, then use `value_at()`, `metrics_at()`, or
   `has_metrics_at()` on the snapshot. Do not issue one HTTP request per value.
4. **Before/after change.** Use `metrics_diff()` instead of manually subtracting two snapshots.

Use `metrics(path=...)` to retrieve all metrics at one exact path.

@note When tests reuse the same service process, `RATE` and `HIST_RATE` values include counts accumulated during
earlier tests. Compare absolute counter values only when the service's counter state is known. Use
@ref pytest_userver.client.ClientMonitor.metrics_diff to check changes caused by an action.


## Paths and labels

Metric paths and labels come from @ref utils::statistics::Writer and the metric's `DumpMetric` implementation.
Indexing a writer appends path components; @ref utils::statistics::Writer::ValueWithLabels attaches labels.
Nested path components do not become labels automatically.

For example, @ref utils::statistics::MinMaxAvg emits `min`, `max`, and `avg` as path components, while
@ref utils::statistics::Percentile emits a `percentile` label such as `p50`.

To inspect the actual paths, labels, types, and values, use
@ref pytest_userver.metrics.MetricsSnapshot.pretty_print or include the snapshot in an assertion's failure message.


## Basic examples

Metric tag definition and usage:

@snippet samples/testsuite-support/src/metrics.cpp metrics definition

@snippet samples/testsuite-support/src/metrics.cpp metrics usage

Checking metrics with a diff:

@snippet samples/testsuite-support/tests/test_metrics.py  metrics diff

Retrieve one metric, or allow it to be absent:

@snippet samples/testsuite-support/tests/test_metrics.py metrics single_metric

@snippet samples/testsuite-support/tests/test_metrics.py metrics single_metric_optional

Metrics with labels:

@snippet samples/testsuite-support/tests/test_metrics.py metrics labels

The @ref pytest_userver.metrics.Metric "Metric" type is hashable and comparable:

@snippet testsuite/tests/metrics/test_metrics.py  values set

Prefer `metrics_diff()` to resetting shared service state. Resetting is available for metrics with a `ResetMetric`
implementation:

@snippet samples/testsuite-support/tests/test_metrics.py metrics reset

* C++ code: @ref samples/testsuite-support/src/metrics.cpp
* C++ header: @ref samples/testsuite-support/src/metrics.hpp
* Testcase: @ref samples/testsuite-support/tests/test_metrics.py


## `MetricsSnapshot`

`MetricsSnapshot` behaves like a mapping from metric paths to sets of `Metric` objects. Prefer plain equality — pytest
shows a useful diff.

The following example constructs expected data and checks several values after slicing:

@snippet testsuite/tests/metrics/test_sliced.py sliced snippet

For `from_layered_dict()`:

- Every top-level key is a complete metric path and is used as-is.
- Nested label mappings use keys in the exact form `'name = value'`, with one space on each side of `=`.
  Nest one mapping per label to describe metrics with several labels.
- A mapping with `bounds`, `buckets`, and `inf` represents a histogram leaf.
- Numeric values are metric leaves. A number directly under a path represents a metric without labels.
- `common_prefix` and `common_labels` remove repetition shared by all expected metrics.

A path mapped to an empty set and an absent path compare equal. For a completely empty result, prefer
`assert not snapshot` over comparison with `MetricsSnapshot.from_layered_dict({})`.

For individual metrics with many labels, direct construction or exact `value_at()` checks may be clearer.

Large expected snapshots can live in JSON fixtures loaded by the `load_json` fixture and passed to
@ref pytest_userver.metrics.MetricsSnapshot.from_layered_dict. Testsuite looks for these JSON files in `static/`
directories.

For example, a layered JSON fixture can contain paths with different label structures:

```json
{
    "worker.processed": {
        "queue = primary": 3,
        "queue = retry": 1
    },
    "worker.errors": {
        "queue = retry": {
            "reason = timeout": 1
        }
    },
    "worker.active": 2
}
```

Use complete snapshot equality when unexpected extra metrics should fail the test. Do not weaken such a comparison
into a few selected-value checks. Conversely, a test for selected values need not compare every metric in the snapshot.


## Exact and subset label matching

@ref pytest_userver.metrics.MetricsSnapshot.value_at uses exact matching when `labels` is supplied: the metric must
have exactly those labels combined with any common labels accumulated by slicing. Pass `{}` to require a metric with
only the common labels, or without labels if none were accumulated. If `labels` is omitted, any labels are accepted,
but the path must identify at most one metric. With `default`, a missing metric returns that value (which may be
`None`); multiple matches still fail.

@ref pytest_userver.metrics.MetricsSnapshot.metrics_at and
@ref pytest_userver.metrics.MetricsSnapshot.has_metrics_at use subset matching via `require_labels`: a metric may
have additional labels. The monitor client's `labels` filter also uses subset matching, including in `single_metric()`
and `single_metric_optional()`. Both single-metric methods fail if multiple metrics match.

An empty snapshot and a value of zero are different: `assert not snapshot` proves no matching metrics exist;
`value_at(path, labels, default=0) == 0` accepts absence or an explicit zero. Choose according to what the test must prove.


## `path` and `prefix`

Use `path='worker.processed'` for one exact path. Use `prefix='worker.'` for a subtree.

`prefix` is a literal string prefix at the monitor endpoint. A trailing dot establishes a path-segment boundary:
`prefix='worker.job'` can also match `worker.jobs`, while `prefix='worker.job.'` selects descendants of `worker.job`.

To assert that a complete subtree is absent, fetch it with `metrics(prefix=...)` and use `assert not snapshot`.
Checking only one leaf with `single_metric_optional()` would miss unexpected sibling metrics.

A broad prefix can include unrelated metrics. Narrow the prefix or fetch once and assert only selected values.


## `metrics_diff()`

Use @ref pytest_userver.client.ClientMonitor.metrics_diff to isolate the effect of an action from values left by
previous tests. The context manager captures the baseline on entry and the current snapshot on exit. Put unrelated
setup before the context.

For `RATE` counters, the diff is the current value minus the baseline. `HIST_RATE` histograms are subtracted per bucket.
By default, `GAUGE` metrics keep their current absolute values. Pass `diff_gauge=True` only when gauge subtraction is
meaningful.

To check the complete result, compare `differ.diff` with an expected
@ref pytest_userver.metrics.MetricsSnapshot using `assert differ.diff == expected`.
For example, suppose an action processes one request from the primary queue without new errors. The number of active
requests after the action is two. With `metrics_diff(prefix='worker.')` and its default `sliced=True`, the expected
diff includes all these metrics:

| Path in the diff | Labels | Baseline | Current | Diff |
| --- | --- | --- | --- | --- |
| `processed` | `queue=primary` | 3 | 4 | 1 |
| `processed` | `queue=retry` | 1 | 1 | 0 |
| `errors` | `queue=retry`, `reason=timeout` | 1 | 1 | 0 |
| `active` | none | 2 | 2 | 2 |

Here `processed` and `errors` are `RATE` counters; `active` is a `GAUGE`. Include the unchanged counters and the current
gauge value when comparing the whole diff.

Use @ref pytest_userver.client.MetricsDiffer.fetch_current for intermediate checkpoints. For a lifecycle that does not
fit one context, call @ref pytest_userver.client.MetricsDiffer.fetch_baseline before the action and `fetch_current()`
after it. Multiple context managers can measure an action across unrelated metric paths without fetching all metrics.

The diff contains the metrics present in the current snapshot, including unchanged metrics with zero counter deltas.
A metric absent from the baseline keeps its current value; a metric absent from the current snapshot is absent from
the diff. Use `default=0` only when a missing metric should mean zero. To omit zero counters and empty histograms in a
snapshot comparison, use @ref pytest_userver.metrics.MetricsSnapshot.without_zero_rates.


## `sliced()` snapshots

@note Pass `sliced=True` to @ref pytest_userver.client.ClientMonitor.metrics "monitor_client.metrics()" to avoid
repeating a common path prefix and labels. See
@ref pytest_userver.metrics.MetricsSnapshot.sliced "MetricsSnapshot.sliced()" for details.

@snippet core/functional_tests/metrics/tests/test_sliced.py sliced functional test

Slicing keeps matching metrics and shortens their paths by whole dot-separated segments. It preserves each `Metric`
object, including its full labels, and remembers common labels so subsequent lookups can omit them. Chained calls
narrow the snapshot further. @ref pytest_userver.metrics.MetricsSnapshot.unsliced restores full paths for the remaining
metrics; it does not restore filtered-out metrics.

`metrics(..., sliced=True)` slices by the requested `path` or `prefix` (with trailing dots removed) and `labels`.
Unlike the endpoint's literal prefix filter, slicing does not match a partial path segment. An exact path becomes `''`.
When calling `snapshot.sliced(prefix, labels)` directly, pass a prefix without a trailing dot, such as `'worker'`.

`metrics_diff()` defaults to `sliced=True`; `metrics()` defaults to `False`. This applies to `differ.value_at()` and to
`differ.baseline`, `differ.current`, and `differ.diff`. Pass `sliced=False` when full paths are clearer.
Use slicing when it reduces repetition; a single lookup usually does not need it.

When comparing a sliced snapshot with `==`, include the common labels in the expected snapshot too. Use
`common_labels` in @ref pytest_userver.metrics.MetricsSnapshot.from_layered_dict to avoid repeating them.


## Custom metrics endpoints

Some tests call a custom HTTP endpoint instead of `monitor_client`. If it supports `format=json`, parse with
`MetricsSnapshot.from_dict()` or `from_json()`. For Solomon JSON, assert on the flat `metrics` list (including the
`sensor` label) without changing production behavior. Prefer `monitor_client.metrics()` when the standard monitor
endpoint is available.


## Compact assertions

Prefer `import pytest_userver.metrics` and qualified names such as `pytest_userver.metrics.MetricsSnapshot`.
This leaves `metrics` available for local snapshot variables. `from pytest_userver import metrics` binds that name to
the module, so a local variable named `metrics` would hide the module.

For metric calls and assertions:

- Keep the complete expression on one line when it fits within 120 columns.
- Otherwise, wrap the call but keep each short argument, especially a labels mapping, on one line.
- Pass labels as the second positional argument to `value_at()`.
- Do not add a trailing comma solely to force a short call onto multiple lines.
- Format ground-truth mappings with one key-value pair per line and trailing commas.

Avoid comments that merely restate the assertion.


## Metrics portability testing

Different monitoring systems impose different limits. @ref server::handlers::TestsControl exposes a portability check:

Call @ref pytest_userver.client.Client.metrics_portability and assert that the returned warnings are empty.

Warnings are grouped by type; filter selectively:

@snippet samples/production_service/tests/test_production.py metrics partial portability

* Testcase: @ref samples/production_service/tests/test_production.py


----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/functional_testing.md | @ref scripts/docs/en/userver/chaos_testing.md ⇨
@htmlonly </div> @endhtmlonly

@example samples/testsuite-support/src/metrics.cpp
@example samples/testsuite-support/src/metrics.hpp
@example samples/testsuite-support/tests/test_metrics.py
