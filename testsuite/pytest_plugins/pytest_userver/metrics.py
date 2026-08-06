"""
Python module that provides helpers for functional testing of metrics with
testsuite; see
@ref scripts/docs/en/userver/functional_testing.md for an introduction.

@ingroup userver_testsuite
"""

from __future__ import annotations

from collections.abc import Mapping
from collections.abc import Set
import dataclasses
import enum
import itertools
import json
import math
import random
from typing import Any
from typing import overload
from typing import TypeAlias
from typing import TypeVar


# @cond
class MetricType(str, enum.Enum):
    """
    The type of individual metric.

    `UNSPECIFIED` compares equal to all `MetricType`s.
    To disable this behavior, use `is` for comparisons.
    """

    UNSPECIFIED = 'UNSPECIFIED'
    GAUGE = 'GAUGE'
    RATE = 'RATE'
    HIST_RATE = 'HIST_RATE'
    # @endcond


@dataclasses.dataclass
class Histogram:
    """
    Represents the value of a HIST_RATE (a.k.a. Histogram) metric.

    Usage example:
    @snippet testsuite/tests/metrics/test_metrics.py  histogram

    Normally obtained from MetricsSnapshot
    """

    bounds: list[float]
    buckets: list[int]
    inf: int

    def count(self) -> int:
        return sum(self.buckets) + self.inf

    def percentile(self, percent: float) -> float:
        return _do_compute_percentile(self, percent)

    # @cond
    def __post_init__(self):
        assert len(self.bounds) == len(self.buckets)
        assert sorted(self.bounds) == self.bounds
        if self.bounds:
            assert self.bounds[0] > 0
            assert self.bounds[-1] != math.inf

    # @endcond


MetricValue: TypeAlias = float | Histogram

T = TypeVar('T')
_MISSING: Any = object()


@dataclasses.dataclass(frozen=True)
class Metric:
    """
    Metric type that contains the `labels: dict[str, str]` and
    `value: int`.

    The type is hashable and comparable:
    @snippet testsuite/tests/metrics/test_metrics.py  values set

    @ingroup userver_testsuite
    """

    labels: dict[str, str]
    value: MetricValue

    # @cond
    # Should not be specified explicitly, for internal use only.
    _type: MetricType = MetricType.UNSPECIFIED
    # @endcond

    def __eq__(self, other: object) -> bool:
        if not isinstance(other, Metric):
            return NotImplemented
        return self.labels == other.labels and self.value == other.value and _type_eq(self._type, other._type)

    def __hash__(self) -> int:
        return hash(_get_labels_tuple(self))

    # @cond
    def __post_init__(self):
        if isinstance(self.value, Histogram):
            assert self._type in (MetricType.HIST_RATE, MetricType.UNSPECIFIED)
        else:
            assert self._type is not MetricType.HIST_RATE

    # For internal use only.
    def type(self) -> MetricType:
        return self._type

    # @endcond


class _MetricsJSONEncoder(json.JSONEncoder):
    def default(self, o):  # pylint: disable=method-hidden
        if isinstance(o, Metric):
            result = {'labels': o.labels, 'value': o.value}
            if o.type() is not MetricType.UNSPECIFIED:
                result['type'] = o.type()
            return result
        elif isinstance(o, Histogram):
            return dataclasses.asdict(o)
        if isinstance(o, set):
            return list(o)
        return super().default(o)


class MetricsSnapshot:
    """
    Snapshot of captured metrics that mimics the dict interface. Metrics have
    the 'dict[str(path), Set[Metric]]' format.

    Example with @ref pytest_userver.client.ClientMonitor.metrics "await monitor_client.metrics(path_prefix, labels)":
    @snippet samples/testsuite-support/tests/test_metrics.py metrics metrics

    There are 3 ways to construct a `MetricsSnapshot`:

    1. The constructor itself, taking a ready `dict[str(path), Set[Metric]]`.
       Useful when dealing with just a few simple metrics, or when generating metrics programmatically,
       or when combining or transforming snapshots. This format is also well-suited for individual metrics
       with a large number of labels.
    2. `from_dict` / `from_json`, taking the flat `json` userver metrics format
       (a list of `{"labels": ..., "value": ...}` per path).
       This is an alternative to the constructor for loading from a JSON file.
       Note that `from_layered_dict` is often more terse and more readable.
    3. `from_layered_dict`, taking a layered dict format that avoids repeating labels for every metric.
       Recommended in tests that have many metrics sharing a label structure.
       Can be written out in code or loaded from a JSON file using `load_json` fixture.

    @ingroup userver_testsuite
    """

    def __init__(
        self,
        values: Mapping[str, Set[Metric]],
        *,
        common_prefix: str = '',
        common_labels: Mapping[str, str] | None = None,
    ):
        """
        @param values Metrics keyed by path, as a `dict[str(path), Set[Metric]]`
            (the same format `MetricsSnapshot` itself exposes via `items()`).
        @param common_prefix If provided, prepended to each path (separated by a dot).
        @param common_labels If provided, these labels are added to every metric,
            merged with (and overridden by) that metric's own labels.
        """
        self._values = _apply_common_prefix_labels(values, common_prefix, common_labels)
        self._sliced_prefix: str | None = None
        self._sliced_labels: Mapping[str, str] = {}

    def __getitem__(self, path: str) -> Set[Metric]:
        """Returns a list of metrics by specified path"""
        return self._values[path]

    def __len__(self) -> int:
        """Returns count of metrics paths"""
        return len(self._values)

    def __iter__(self):
        """Returns a (path, list) iterable over the metrics"""
        return self._values.__iter__()

    def __contains__(self, path: str) -> bool:
        """
        Returns True if metric with specified path is in the snapshot, False otherwise.
        """
        return path in self._values

    def __eq__(self, other: object) -> bool:
        """
        Compares the snapshot with a dict of metrics or with another
        snapshot. A path mapped to an empty set of metrics is treated the
        same as an absent path.
        """
        if isinstance(other, MetricsSnapshot):
            other_values: Mapping[str, Set[Metric]] = other._values
        elif isinstance(other, Mapping):
            other_values = other
        else:
            return NotImplemented
        return _drop_empty_paths(self._values) == _drop_empty_paths(other_values)

    def __repr__(self) -> str:
        return self._values.__repr__()

    def __str__(self) -> str:
        return self.pretty_print()

    def get(self, path: str, default=None):
        """
        Returns an list of metrics by path or default if there's no such path
        """
        return self._values.get(path, default)

    def items(self):
        """Returns a (path, list) iterable over the metrics"""
        return self._values.items()

    def keys(self):
        """Returns an iterable over paths of metrics"""
        return self._values.keys()

    def values(self):
        """Returns an iterable over lists of metrics"""
        return self._values.values()

    def sliced(
        self,
        prefix: str | None,
        labels: dict[str, str] | None = None,
        /,
    ) -> MetricsSnapshot:
        """
        Returns a new MetricsSnapshot restricted to the metrics whose path starts with `prefix` as a whole
        '.'-separated segment, and, if `labels` is given, whose labels are a superset of `labels` (same
        subset-match semantics as `require_labels` in `metrics_at`).

        `prefix` may be:

        * A non-empty string: matches paths equal to `prefix`, or starting with `prefix` followed by a whole
          '.'-separated segment boundary (`prefix='a.b'` matches paths `'a.b'` and `'a.b.c'`, but not `'a.bc'`).
          The matched `prefix` (and the following '.', if any) is stripped from the start of every surviving
          metric's path, so e.g. slicing `'a.b.c'` by `prefix='a.b'` makes it accessible as `'c'`, and slicing
          `'a.b'` by `prefix='a.b'` makes it accessible as `''`.
        * `''`: matches paths equal to `''`, or starting with a literal leading `'.'`; the leading `'.'` is
          stripped. Only relevant for the rare case of a metric path that itself starts with a dot.
        * `None`: matches every path unconditionally. Use this to filter by `labels` only.

        Calling `sliced()` several times in a row composes: each `prefix` is matched against the
        already-stripped paths of the previous `sliced()` call, and `labels` requirements accumulate.

        Slicing only ever affects *filtering* (which metrics are visible, and under which path): it never
        touches the `Metric` objects themselves. `metrics_at()`, `value_at()` and iteration over a sliced
        snapshot all keep returning the exact same, untouched `Metric` objects (same labels, value, identity),
        just possibly under a shorter path and/or a smaller surrounding set.

        Intended use: carve out a small, closed slice of a snapshot (e.g. one metric path with a handful of
        varying labels) to compare it with `==` against a compact expected snapshot, or to look up several
        label combinations with `value_at`/`metrics_at` without repeating the common prefix and labels in every call.

        @throws AssertionError if stripping `prefix` from a metric path would leave an empty remainder,
            e.g. path 'a.b.' sliced by prefix='a.b', which would be indistinguishable from path 'a.b'.

        @snippet testsuite/tests/metrics/test_sliced.py sliced snippet
        """
        result: dict[str, set[Metric]] = {}
        for path, metric_set in self._values.items():
            if prefix is None:
                remainder = path
            else:
                remainder = _strip_prefix_segment(path, prefix)
                if remainder is None:
                    continue

            if labels:
                metric_set = {metric for metric in metric_set if labels.items() <= metric.labels.items()}
                if not metric_set:
                    continue

            result.setdefault(remainder, set())
            result[remainder] |= metric_set

        sliced_snapshot = MetricsSnapshot(result)
        if prefix is None:
            sliced_snapshot._sliced_prefix = self._sliced_prefix
        elif self._sliced_prefix is None:
            sliced_snapshot._sliced_prefix = prefix
        else:
            sliced_snapshot._sliced_prefix = f'{self._sliced_prefix}.{prefix}'
        sliced_snapshot._sliced_labels = {**self._sliced_labels, **labels} if labels else self._sliced_labels
        return sliced_snapshot

    def unsliced(self) -> MetricsSnapshot:
        """
        Returns a new MetricsSnapshot with the `prefix` accumulated from the preceding (possibly chained)
        `sliced()` call(s) prepended back to every surviving metric's path. Does not mutate `self`.

        Metrics that were filtered out by `sliced()` (because their path did not match `prefix`, or their
        labels did not match `labels`) do NOT come back: `unsliced()` only restores the *path* of what
        remains in the snapshot, it does not undo the filtering itself.

        If this snapshot was never `sliced()` (i.e. `self` is the original snapshot, or the result of
        operations other than `sliced()`), returns an equivalent snapshot unchanged.
        """
        if self._sliced_prefix is None:
            return MetricsSnapshot(self._values)
        prefix = self._sliced_prefix
        result = {(f'{prefix}.{path}' if path else prefix): metric_set for path, metric_set in self._values.items()}
        return MetricsSnapshot(result)

    @overload
    def value_at(
        self,
        path: str,
        labels: dict[str, str] | None = None,
    ) -> MetricValue: ...

    @overload
    def value_at(
        self,
        path: str,
        labels: dict[str, str] | None,
        *,
        default: T,
    ) -> MetricValue | T: ...

    def value_at(
        self,
        path: str,
        labels: dict[str, str] | None = None,
        *,
        default: Any = _MISSING,
    ) -> MetricValue | Any:
        """
        Returns a single metric value at specified path. If a dict of labels
        is provided, does en exact match of labels (i.e. {} stands for no
        labels; {'a': 'b', 'c': 'd'} matches only {'a': 'b', 'c': 'd'} or
        {'c': 'd', 'a': 'b'} but neither match {'a': 'b'} nor {'a': 'b', 'c': 'd', 'e': 'f'}).

        If `default` is provided, it is returned instead of asserting when
        the metric is not found.

        @throws AssertionError if not one metric by path and no `default` is given

        @snippet samples/testsuite-support/tests/test_metrics.py metrics metrics
        """
        entry = self.get(path, set())
        assert entry or default is not _MISSING, f'No metrics found by path "{path}"' + (
            f' after slicing "{self._sliced_prefix}"' if self._sliced_prefix is not None else ''
        )

        if labels is not None:
            full_labels = {**self._sliced_labels, **labels} if self._sliced_labels else labels
            filtered_entries = {x for x in entry if x.labels == full_labels}
            assert filtered_entries or default is not _MISSING, (
                f'No metrics found by path "{path}" and labels {full_labels}. Possible values: {entry}'
            )
            assert len(filtered_entries) <= 1, (
                f'Multiple metrics found by path "{path}" and labels {full_labels}: {filtered_entries}'
            )
            entry = filtered_entries
        else:
            assert len(entry) <= 1, f'Multiple metrics found by path "{path}": {entry}'

        if default is not _MISSING and not entry:
            return default
        return next(iter(entry)).value

    def metrics_at(
        self,
        path: str,
        require_labels: dict[str, str] | None = None,
    ) -> list[Metric]:
        """
        Metrics path must exactly equal the given `path`.
        A required subset of labels is specified by `require_labels`
        Example:
        require_labels={'a':'b', 'c':'d'}
        { 'a':'b', 'c':'d'} - exact match
        { 'a':'b', 'c':'d', 'e': 'f', 'h':'k'} - match
        { 'a':'x', 'c':'d'} - no match, incorrect value for label 'a'
        { 'a' : 'b'} - required label not found

        @snippet samples/testsuite-support/tests/test_metrics.py metrics metrics
        """
        entry = self.get(path, set())
        full_require_labels = (
            {**self._sliced_labels, **require_labels} if require_labels is not None else self._sliced_labels or None
        )

        if full_require_labels is not None:
            return [metric for metric in entry if full_require_labels.items() <= metric.labels.items()]
        else:
            return list(entry)

    def has_metrics_at(
        self,
        path: str,
        require_labels: dict[str, str] | None = None,
    ) -> bool:
        # metrics_with_labels returns list, and pythonic way to check if list
        # is empty is like this:
        return bool(self.metrics_at(path, require_labels))

    def assert_equals(
        self,
        other: Mapping[str, Set[Metric]],
        *,
        ignore_zeros: bool = False,
    ) -> None:
        """
        @deprecated Use `==` operator instead, which produces a nice diff
        automatically via `pytest_assertrepr_compare`. To ignore zero-rate
        metrics, use `without_zero_rates()` on the snapshots before comparing.
        """
        lhs = _flatten_snapshot(self, ignore_zeros=ignore_zeros)
        rhs = _flatten_snapshot(other, ignore_zeros=ignore_zeros)
        assert lhs == rhs, _diff_metric_snapshots(lhs, rhs, ignore_zeros)

    def without_zero_rates(self) -> MetricsSnapshot:
        """
        Returns a new snapshot with "empty" RATE and HIST_RATE metrics
        removed: a RATE metric is removed if its value is zero, a HIST_RATE
        metric is removed if its histogram has zero count in every bucket
        and in `inf`. GAUGE (and untyped) metrics are kept as-is, because a
        zero GAUGE value can be meaningful.
        """
        return MetricsSnapshot(
            _drop_empty_paths({
                path: {metric for metric in metric_set if not _is_zero_rate_or_histogram(metric)}
                for path, metric_set in self._values.items()
            }),
        )

    def pretty_print(self) -> str:
        """
        Multiline linear print:
          path:  (label=value),(label=value) TYPE VALUE
          path:  (label=value),(label=value) TYPE VALUE
        Usage:
        @code
         assert 'some.thing.sensor' in metric, metric.pretty_print()
        @endcode
        """

        def _iterate_over_mset(path, mset):
            """print (pretty) one metrics set - for given path"""
            result = []
            for metric in sorted(mset, key=lambda x: _get_labels_tuple(x)):
                result.append(_format_metric_line(path, metric))
            return result

        # list of lists [ [ string1, string2, string3],
        #                 [string4, string5, string6] ]
        data_for_every_path = [_iterate_over_mset(path, mset) for path, mset in self._values.items()]
        # use itertools.chain to flatten list
        # [ string1, string2, string3, string4, string5, string6 ]
        # and join to convert it to one multiline string
        return '\n'.join(itertools.chain(*data_for_every_path))

    @staticmethod
    def from_dict(data: Mapping[str, Any]) -> MetricsSnapshot:
        """
        Construct MetricsSnapshot from a JSON dict in the `json` userver metrics format.
        """
        json_data = {
            str(path): {
                Metric(
                    labels=element['labels'],
                    value=_parse_metric_value(element['value']),
                    _type=MetricType[element.get('type', 'UNSPECIFIED')],
                )
                for element in metrics_list
            }
            for path, metrics_list in data.items()
        }
        return MetricsSnapshot(json_data)

    @staticmethod
    def from_json(json_str: str) -> MetricsSnapshot:
        """
        Construct MetricsSnapshot from a JSON string in the `json` userver metrics format.
        """
        return MetricsSnapshot.from_dict(json.loads(json_str))

    @staticmethod
    def from_layered_dict(
        data: Mapping[str, Any],
        *,
        common_prefix: str = '',
        common_labels: Mapping[str, str] | None = None,
    ) -> MetricsSnapshot:
        """
        Construct MetricsSnapshot from a layered dict format that avoids
        repeating a label's name for every metric that only differs by
        that label's value.

        Top-level keys of `data` are metric paths, used as-is. Within a
        path's value, each dict key names a label as `'name = value'`
        (with exactly one space on each side of `=`: everything before is
        the label name, everything after is its value); the corresponding
        child value is interpreted the same way recursively, so several
        labels can be layered one inside another. A dict with `bounds` and
        `buckets` keys is a leaf value instead of being recursed into,
        parsed as a `Histogram`; any other non-dict value is a plain leaf
        metric value.

        If `common_prefix` is provided, it is prepended to each path
        (separated by a dot) so that paths in `data` can omit a shared prefix.

        If `common_labels` is provided, these labels are added to every
        metric in the snapshot, merged with any labels from the layered dict
        structure.

        Example: `{'a': {'x = foo': 1, 'x = bar': 2}}` is equivalent to
        `MetricsSnapshot({'a': {Metric({'x': 'foo'}, 1), Metric({'x': 'bar'}, 2)}})`.
        """
        return MetricsSnapshot(
            {path: _collect_layered_metrics(node, {}) for path, node in data.items()},
            common_prefix=common_prefix,
            common_labels=common_labels,
        )

    def to_json(self) -> str:
        """
        Serialize to a JSON string
        """
        return json.dumps(
            # Shuffle to disallow depending on the received metrics order.
            {path: random.sample(list(metrics), len(metrics)) for path, metrics in self._values.items()},
            cls=_MetricsJSONEncoder,
        )


def _apply_common_prefix_labels(
    values: Mapping[str, Set[Metric]],
    common_prefix: str,
    common_labels: Mapping[str, str] | None,
) -> Mapping[str, Set[Metric]]:
    if common_labels:
        prefix = f'{common_prefix}.' if common_prefix else ''
        return {
            f'{prefix}{path}': {
                dataclasses.replace(metric, labels={**common_labels, **metric.labels}) for metric in metric_set
            }
            for path, metric_set in values.items()
        }
    if common_prefix:
        return {f'{common_prefix}.{path}': metric_set for path, metric_set in values.items()}
    return values


def _drop_empty_paths(values: Mapping[str, Set[Metric]]) -> dict[str, Set[Metric]]:
    return {path: metric_set for path, metric_set in values.items() if metric_set}


def _strip_prefix_segment(path: str, prefix: str) -> str | None:
    assert not prefix.endswith('.'), f'prefix "{prefix}" must not end with "."'
    if path == prefix:
        return ''
    dotted_prefix = f'{prefix}.'
    if path.startswith(dotted_prefix):
        remainder = path.removeprefix(dotted_prefix)
        # An empty remainder here would be indistinguishable from the `path == prefix` case above
        # (e.g. path 'a.' sliced by prefix 'a' would otherwise collide with path 'a'). A remainder
        # starting with '.' is fine, though: it just means `path` had an empty '.'-segment right
        # after `prefix` (e.g. path 'a..b' sliced by prefix 'a' yields '.b', still distinguishable
        # from path 'a.b' sliced by prefix 'a', which yields 'b').
        assert remainder, (
            f'Metric path "{path}" becomes empty right after prefix "{prefix}", '
            f'which would make prefix matching ambiguous. This looks like a malformed metric path.'
        )
        return remainder
    return None


def _is_zero_rate_or_histogram(metric: Metric) -> bool:
    if isinstance(metric.value, Histogram):
        return metric.value.count() == 0
    return metric.type() == MetricType.RATE and metric.value == 0


def _type_eq(lhs: MetricType, rhs: MetricType) -> bool:
    return lhs == rhs or lhs == MetricType.UNSPECIFIED or rhs == MetricType.UNSPECIFIED  # noqa: PLR1714


def _get_labels_tuple(metric: Metric) -> tuple[tuple[str, str], ...]:
    """Returns labels as a tuple of sorted items"""
    return tuple(sorted(metric.labels.items()))


def _format_metric_line(path: str, metric: Metric, *, forced_type: MetricType | None = None) -> str:
    """Formats a single metric line as "path: (labels) TYPE VALUE", skipping the labels part (and the extra
    space) entirely if there are no labels. `forced_type` overrides `metric`'s own type, if given"""
    labels_str = ','.join(f'({key}={label_value})' for key, label_value in _get_labels_tuple(metric))
    metric_type = forced_type if forced_type is not None else metric.type()
    parts = [f'{path}:', labels_str, metric_type.value, str(metric.value)]
    return ' '.join(part for part in parts if part)


def stringify_snapshot_for_diff(
    values: Mapping[str, Set[Metric]],
    /,
    *,
    other: Mapping[str, Set[Metric]],
) -> set[str]:
    """
    Renders a snapshot as a set of "path: (labels) TYPE VALUE" strings
    (the same format as `MetricsSnapshot.pretty_print`), suitable for use
    with a generic set-based diff (e.g. testsuite's `CompareVisitor`
    machinery).

    A metric with an UNSPECIFIED type has its type resolved from the
    matching (by path and labels) metric in `other`, if any, mirroring the
    wildcard-matching behavior of `Metric.__eq__`.
    """
    other_snapshot = other if isinstance(other, MetricsSnapshot) else MetricsSnapshot(other)
    return {
        _format_metric_line(path, metric, forced_type=_resolve_type(path, metric, other_snapshot))
        for path, metric_set in values.items()
        for metric in metric_set
    }


def _resolve_type(path: str, metric: Metric, other_snapshot: MetricsSnapshot) -> MetricType:
    if metric.type() != MetricType.UNSPECIFIED:
        return metric.type()
    for other_metric in other_snapshot.metrics_at(path, require_labels=metric.labels):
        if other_metric.labels == metric.labels and other_metric.type() != MetricType.UNSPECIFIED:
            return other_metric.type()
    return metric.type()


def _do_compute_percentile(hist: Histogram, percent: float) -> float:
    # This implementation is O(hist.count()), which is less than perfect.
    # So far, this was not a big enough pain to rewrite it.
    value_lists = [[bound] * bucket for (bucket, bound) in zip(hist.buckets, hist.bounds, strict=True)] + [
        [math.inf] * hist.inf
    ]
    values = [item for sublist in value_lists for item in sublist]

    # Implementation taken from:
    # https://stackoverflow.com/a/2753343/5173839
    if not values:
        return 0
    pivot = (len(values) - 1) * percent
    floor = math.floor(pivot)
    ceil = math.ceil(pivot)
    if floor == ceil:
        return values[int(pivot)]
    part1 = values[int(floor)] * (ceil - pivot)
    part2 = values[int(ceil)] * (pivot - floor)
    return part1 + part2


def _is_histogram_dict(node: Any) -> bool:
    return isinstance(node, dict) and 'bounds' in node and 'buckets' in node


_LABEL_SEPARATOR = ' = '


def _collect_layered_metrics(node: Any, labels: dict[str, str]) -> set[Metric]:
    if isinstance(node, dict) and not _is_histogram_dict(node):
        result = set()
        for key, child in node.items():
            assert _LABEL_SEPARATOR in key, f"Expected a label key like 'name = value', got '{key}'"
            label_name, label_value = key.split(_LABEL_SEPARATOR, 1)
            assert not label_name.endswith(' ') and not label_value.startswith(' '), (
                f"Expected exactly one space on each side of '=' in a label key, got '{key}'"
            )
            result |= _collect_layered_metrics(child, {**labels, label_name: label_value})
        return result
    return {Metric(dict(labels), _parse_metric_value(node))}


def _parse_metric_value(value: Any) -> MetricValue:
    if isinstance(value, dict):
        return Histogram(
            bounds=value['bounds'],
            buckets=value['buckets'],
            inf=value['inf'],
        )
    elif isinstance(value, float):
        return value
    elif isinstance(value, int):
        return value
    else:
        raise Exception(f'Failed to parse metric value from {value!r}')


_FlattenedSnapshot: TypeAlias = Set[tuple[str, Metric]]


def _flatten_snapshot(values, ignore_zeros: bool) -> _FlattenedSnapshot:
    return {
        (path, metric)
        for path, metrics in values.items()
        for metric in metrics
        if metric.value != 0 or not ignore_zeros
    }


def _diff_metric_snapshots(
    lhs: _FlattenedSnapshot,
    rhs: _FlattenedSnapshot,
    ignore_zeros: bool,
) -> str:
    def extra_metrics_message(extra, base):
        return [
            f'    path={path!r} labels={metric.labels!r} value={metric.value}'
            for path, metric in sorted(extra, key=lambda pair: pair[0])
            if (path, metric) not in base
        ]

    if ignore_zeros:
        lines = ['left.assert_equals(right, ignore_zeros=True) failed']
    else:
        lines = ['left.assert_equals(right) failed']
    actual_extra = extra_metrics_message(lhs, rhs)
    if actual_extra:
        lines.append('  extra in left:')
        lines += actual_extra

    actual_gt = extra_metrics_message(rhs, lhs)
    if actual_gt:
        lines.append('  missing in left:')
        lines += actual_gt

    return '\n'.join(lines)
