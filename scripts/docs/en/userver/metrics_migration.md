# Migrating tests from the removed legacy metrics API

This guide explains how to migrate tests that used the removed
`ClientMonitor.get_metric()` and `ClientMonitor.get_metrics()` methods to
`ClientMonitor.metrics()`, `ClientMonitor.single_metric()`,
`ClientMonitor.single_metric_optional()`, `MetricsSnapshot`, and
`ClientMonitor.metrics_diff()`.

The API docstrings are the source of truth. See @ref pytest_userver.client.ClientMonitor and
@ref pytest_userver.metrics.MetricsSnapshot.

## 1. Understand the format change

The removed methods requested `format=internal`. That format is a nested JSON object without metric types. Labels are
encoded indirectly through `$meta: {solomon_children_labels: <label-name>}`: children of that node become values of the
specified label and disappear from the metric path. A node marked with `SolomonSkip` also disappears from the path, but
its children do not become label values.

The current API requests `format=json` and returns a flat collection of metric series. Every series has a path, labels,
a value, and a type. Tests receive it as a @ref pytest_userver.metrics.MetricsSnapshot "MetricsSnapshot".

A literal dot inside one key of the legacy nested format may become an underscore in the JSON path. For example, the
legacy key `"requests.success"` may become the path segment `requests_success`; this differs from dots that separate
nested object levels. Inspect the actual JSON snapshot when the resulting path is unclear.

## 2. Choose the replacement by test intent

Classify each legacy call before rewriting it:

1. **Complete ground-truth comparison.** Fetch one snapshot with `metrics(prefix=...)` and compare it with
   `MetricsSnapshot.from_layered_dict(...)` or a directly constructed `MetricsSnapshot`.
2. **One exact series.** Use `single_metric()` when the series must exist, or `single_metric_optional()` when absence is
   expected and meaningful.
3. **Several values from one subtree.** Fetch once with `metrics(prefix=...)`, then use `value_at()`, `metrics_at()`, or
   `has_metrics_at()` on the snapshot. Do not issue one HTTP request per value.
4. **Before/after change.** Use `metrics_diff()` instead of manually fetching and subtracting two snapshots.

Preserve the width of the original assertion. A whole legacy subtree normally maps to `metrics(prefix=...)`; one leaf
maps to `path=...` or `single_metric()`; several selected values map to one broad fetch followed by client-side checks.
A narrower replacement can miss unexpected series, while a wider one can include unrelated metrics.

## 3. Derive paths and labels correctly

When possible, inspect the metric writer or registration:

- `SolomonChildrenAreLabelValues(node, "label")` turns the children of `node` into values of `label`; those child names
  disappear from the path. Apply this rule independently at every marked nesting level.
- `SolomonSkip(node)` removes the node name from the path without turning its children into labels.
- Ordinary nested JSON keys remain path components.

Plain nesting is not evidence of labels. For example, a writer equivalent to
`writer["jobs"][kind]["attempt"][number]` without label metadata produces paths such as
`jobs.email.attempt.1`, not `jobs` with `kind` and `attempt` labels.

`MinMaxAvg` and `Percentile` also have different JSON representations:

- `MinMaxAvg` fields are path components, for example `latency.1min.min`, `latency.1min.max`, and
  `latency.1min.avg`.
- Percentiles are labels, for example path `latency.1min` with label `{'percentile': 'p50'}`.

If static inspection is inconclusive, temporarily print the actual snapshot:

```python
snapshot = await monitor_client.metrics(prefix='worker.requests')
assert False, snapshot.pretty_print()
```

Run the focused test normally:

```shell
pytest path/to/test_file.py::test_metric
```

The assertion output contains each path, label set, type, and value. Remove the temporary failing assertion after
updating the test.

## 4. Use `MetricsSnapshot` for complete comparisons

`MetricsSnapshot` behaves like a mapping from metric paths to sets of `Metric` objects. Prefer plain equality because
pytest provides a useful snapshot diff:

```python
snapshot = await monitor_client.metrics(prefix='worker')
assert snapshot == pytest_userver.metrics.MetricsSnapshot.from_layered_dict({
    'worker.processed': {
        'queue = primary': 3,
        'queue = retry': 1,
    },
})
```

For `from_layered_dict()`:

- Every top-level key is a complete metric path and is used as-is.
- Every nested key must be a label in the exact form `'name = value'`, with one space on each side of `=`.
- A mapping with `bounds` and `buckets` is a histogram leaf.
- Every other non-mapping value is a metric leaf.
- `common_prefix` and `common_labels` can remove repetition shared by all expected series.

A path mapped to an empty set and an absent path compare equal. For a completely empty result, prefer
`assert not snapshot` over comparison with `MetricsSnapshot.from_layered_dict({})`.

`from_layered_dict()` supports any number of nested labels. For isolated series with many labels, direct construction
or exact `value_at()` checks may be clearer:

```python
snapshot = await monitor_client.metrics(path='worker.processed')
assert snapshot.value_at(
    'worker.processed',
    {'queue': 'primary', 'region': 'west', 'result': 'ok'},
) == 3
```

Do not keep legacy `$meta` data and convert it at runtime. Rewrite expected data directly in the current snapshot
format so that the test documents the real path-and-label model.

## 5. Use exact and subset label matching deliberately

`value_at(path, labels)` uses exact label matching. The series must have exactly the supplied labels. Without `default`,
it asserts that exactly one series was found; with `default`, it returns that value when no series was found. The
default may be any value, including `None`.

```python
value = snapshot.value_at('worker.processed', {'queue': 'primary'}, default=0)
```

`metrics_at(path, require_labels=...)` and `has_metrics_at(path, require_labels=...)` use subset matching. A series may
have additional labels:

```python
assert snapshot.has_metrics_at('worker.processed', require_labels={'queue': 'primary'})
```

Do not replace a subset existence check with `value_at(..., default=None)` unless the complete label set is known.
Conversely, use `value_at()` when the test needs one exact value.

An empty snapshot and a value of zero are different assertions. `assert not snapshot` proves that no matching series
exist. `value_at(path, labels, default=0) == 0` accepts either an absent series or a present series whose value is zero.
Choose according to the original test semantics.

## 6. Respect `path` and `prefix` semantics

Use `path='worker.processed'` for one exact metric path. Use `prefix='worker.'` for a subtree.

`prefix` is a literal string prefix at the monitor endpoint. A trailing dot establishes a path-segment boundary:
`prefix='worker.job'` can also match `worker.jobs`, while `prefix='worker.job.'` selects descendants of `worker.job`.

A legacy assertion that a complete subtree is absent must remain a subtree assertion:

```python
snapshot = await monitor_client.metrics(prefix='worker.errors.')
assert not snapshot
```

Replacing it with `single_metric_optional('worker.errors.timeout') is None` would miss another unexpected series such
as `worker.errors.protocol`.

A broad prefix can include generated or neighboring metrics that the legacy test discarded manually. Either narrow the
prefix to the intended namespace or fetch once and assert only the selected values. Do not claim complete snapshot
coverage when the test checks only a few leaves.

## 7. Use `metrics_diff()` for before/after assertions

For one measured action, use the context manager:

```python
async with monitor_client.metrics_diff(prefix='worker.', sliced=False) as differ:
    await perform_action()

assert differ.value_at('worker.processed', {'queue': 'primary'}, default=0) == 1
```

The context manager fetches the baseline on entry and the current snapshot on exit. Put unrelated setup before the
context so that fixture activity, mock configuration, or test-data preparation does not affect the measured interval.

A single scope may contain multiple `metrics_diff()` context managers. Use this when the measured metrics have no useful
common path prefix or labels and fetching every service metric would be wasteful. Each differ then requests only its own
path or prefix while all baselines are captured before the action and all current values after it:

```python
async with (
    monitor_client.metrics_diff(path='orders.created') as created,
    monitor_client.metrics_diff(path='payments.failed') as failed,
):
    await perform_action()

assert created.value_at() == 1
assert failed.value_at() == 0
```

By default, numeric subtraction is applied to `RATE` metrics. A `GAUGE` normally retains its current absolute value. If
the test intentionally needs a gauge delta, pass `diff_gauge=True`:

```python
async with monitor_client.metrics_diff(prefix='queue.', diff_gauge=True) as differ:
    await enqueue_item()

assert differ.value_at('depth') == 1
```

Use this only when subtraction is meaningful. Plain atomics and `RecentPeriod` values commonly appear as `GAUGE`
metrics even when application code increments them like counters.

For an intermediate checkpoint, call `fetch_current()`. It stores the new current snapshot and updates the diff:

```python
await differ.fetch_current()
assert differ.value_at('processed', default=0) == 2
```

For a lifecycle that does not map cleanly onto one context-manager scope, use `fetch_baseline()` before the measured
action and `fetch_current()` after it. For the usual single-action lifecycle, prefer `async with`.

Server-side `labels=` filtering in `metrics_diff()` can be unsuitable for metrics whose labels are produced by Solomon
metadata. If it excludes the series unexpectedly, fetch by path or prefix and filter in `value_at()`:

```python
async with monitor_client.metrics_diff(prefix='worker.', sliced=False) as differ:
    await perform_action()

assert differ.value_at('worker.processed', {'queue': 'primary'}, default=0) == 1
```

## 8. Avoid missing-series pitfalls

A diff snapshot may omit a series that did not change. If an unchanged or non-incremented series semantically means
zero, pass `default=0`:

```python
for result in ('ok', 'error'):
    assert differ.value_at('requests', {'result': result}, default=0) == expected[result]
```

Do not add `default=0` when the test must prove that the series exists. Likewise, do not use it when absence and an
explicit zero have different meanings.

When migrating a shared helper, update every caller and every stored value consistently. Comparing a legacy mapping
with a `MetricsSnapshot` fails even if the contained numbers look equivalent. Shared fixtures that manually fetch and
subtract metrics should normally be replaced by `metrics_diff()` at the call site.

## 9. Use `sliced()` to remove repeated prefixes and labels

If several checks repeat a long common path or common labels, request a sliced snapshot:

```python
snapshot = await monitor_client.metrics(prefix='worker.external_calls', sliced=True)
assert snapshot.value_at('success', {'peer': 'billing', 'method': 'charge'}) > 0
assert snapshot.value_at('error', {'peer': 'billing', 'method': 'charge'}, default=0) == 0
```

`metrics(..., sliced=True)` is equivalent to calling `.sliced(path or prefix, labels)` on the returned snapshot.
`MetricsSnapshot.sliced(prefix, labels)` keeps matching series, removes the common path prefix, and remembers common
labels for later exact lookups. Calls may be chained to narrow a snapshot in stages. `unsliced()` restores full paths of
the remaining series; it does not restore series filtered out by slicing.

Slicing uses whole dot-separated path segments, unlike the server-side literal `prefix` filter. Do not add slicing for a
single lookup when it does not reduce repetition.

With `sliced=True` and an exact `path=...`, a nonexistent path produces an empty snapshot. A later `value_at('')` error
includes the sliced prefix, which is usually sufficient to diagnose the mismatch.

`metrics_diff()` uses `sliced=True` by default, unlike `metrics()`. Therefore paths passed to `differ.value_at()` are
normally relative to the requested path or prefix. Pass `sliced=False` when retaining full paths is clearer.

## 10. Handle custom metrics endpoints deliberately

Some tests call a custom HTTP endpoint that exposes userver metrics instead of using `monitor_client`. First determine
which format that endpoint intentionally supports:

- A custom endpoint that supports `format=json` may be parsed with `MetricsSnapshot.from_dict()` or
  `MetricsSnapshot.from_json()`.
- Solomon JSON is already a flat list of series with explicit labels, including the `sensor` label. It is not the
  removed hierarchical `format=internal` representation. Do not change production endpoint behavior merely to use
  `MetricsSnapshot` in a test; request or fetch Solomon explicitly and assert on its `metrics` list instead.

```python
response = await service_client.get('/custom-metrics', params={'format': 'solomon'})
assert response.status == 200
series = [item for item in response.json()['metrics'] if item['labels']['sensor'] == 'worker.processed']
assert len(series) == 1
```

Preserve the endpoint's path, prefix, and label filtering semantics. Prefer `monitor_client.metrics()` when the standard
monitor endpoint is available and supports `format=json`.

## 11. Keep metrics assertions compact

Use `import pytest_userver.metrics` and the qualified names
`pytest_userver.metrics.MetricsSnapshot` and `pytest_userver.metrics.Metric`. This leaves `metrics` available as a local
snapshot variable. If a file already imports `from pytest_userver import metrics`, avoid shadowing that module; name the
local value `snapshot` instead.

For `metrics()`, `metrics_diff()`, `single_metric()`, `single_metric_optional()`, `value_at()`, `metrics_at()`,
`sliced()`, and `from_layered_dict()`:

- Keep the complete expression on one line when it fits within 120 columns.
- Otherwise, wrap the call but keep each short argument, especially a labels mapping, on one line.
- Pass labels as the second positional argument to `value_at()`.
- Do not add a trailing comma solely to force a short call onto multiple lines.
- Format ground-truth mappings with one key-value pair per line and trailing commas.

```python
assert snapshot.value_at('worker.processed', {'queue': 'primary'}, default=0) == 3

assert snapshot.value_at(
    'worker.requests.with.a.long.path.that.makes.the.complete.assertion.exceed.the.limit',
    {'queue': 'primary', 'region': 'west', 'result': 'ok'},
    default=0,
) == expected_value
```

Construct expected snapshots next to their data. Do not pass a raw legacy mapping through a distant helper and convert
it later. Avoid comments that merely restate the assertion.

## 12. Validate the migration

A service created from the userver service template registers one CTest test named `testsuite-service_template` via
`userver_testsuite_add_simple()`. Select that CTest test with `-R`, and pass a pytest filter inside it through
`PYTEST_ADDOPTS`. For example, with `tests/test_postgres.py`:

```shell
make build-debug
PYTEST_ADDOPTS='-k test_postgres' ctest --test-dir build-debug -V -R 'testsuite-service_template'
PYTEST_ADDOPTS='-k test_db_updates' ctest --test-dir build-debug -V -R 'testsuite-service_template'
```

The CTest `-R` expression selects the single testsuite test. The first pytest `-k` expression selects the test module by
its filename stem; the second selects one test function inside that testsuite test. Because `-k` is a keyword expression
rather than an exact node-id selector, use a distinctive module or function name and check the collection summary. Run
the whole modified module after the focused function.

Read snapshot diffs carefully: they show the actual path, labels, type, and value. Verify that the migrated test
performs the same number of monitor requests, covers the same set of series, and contains no temporary diagnostic
assertions.

----------

@htmlonly <div class="bottom-nav"> @endhtmlonly
⇦ @ref scripts/docs/en/userver/functional_testing.md | @ref scripts/docs/en/userver/chaos_testing.md ⇨
@htmlonly </div> @endhtmlonly
