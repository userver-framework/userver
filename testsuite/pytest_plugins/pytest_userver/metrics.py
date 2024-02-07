"""
Python module that provides helpers for functional testing of metrics with
testsuite; see
@ref scripts/docs/en/userver/functional_testing.md for an introduction.

@ingroup userver_testsuite
"""

import dataclasses
import enum
import itertools
import json
import math
import random
import typing


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
    @snippet testsuite/tests/test_metrics.py  histogram

    Normally obtained from MetricsSnapshot
    """

    bounds: typing.List[float]
    buckets: typing.List[int]
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


MetricValue = typing.Union[float, Histogram]


@dataclasses.dataclass(frozen=True)
class Metric:
    """
    Metric type that contains the `labels: typing.Dict[str, str]` and
    `value: int`.

    The type is hashable and comparable:
    @snippet testsuite/tests/test_metrics.py  values set

    @ingroup userver_testsuite
    """

    labels: typing.Dict[str, str]
    value: MetricValue

    # @cond
    # Should not be specified explicitly, for internal use only.
    _type: MetricType = MetricType.UNSPECIFIED
    # @endcond

    def __eq__(self, other: typing.Any) -> bool:
        if not isinstance(other, Metric):
            return NotImplemented
        return (
            self.labels == other.labels
            and self.value == other.value
            and _type_eq(self._type, other._type)
        )

    def __hash__(self) -> int:
        return hash(_get_labels_tuple(self))

    # @cond
    def __post_init__(self):
        if isinstance(self.value, Histogram):
            assert (
                self._type == MetricType.HIST_RATE
                or self._type == MetricType.UNSPECIFIED
            )
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
    the 'Dict[str(path), Set[Metric]]' format.

    @snippet samples/testsuite-support/tests/test_metrics.py metrics labels

    @ingroup userver_testsuite
    """

    def __init__(self, values: typing.Mapping[str, typing.Set[Metric]]):
        self._values = values

    def __getitem__(self, path: str) -> typing.Set[Metric]:
        """ Returns a list of metrics by specified path """
        return self._values[path]

    def __len__(self) -> int:
        """ Returns count of metrics paths """
        return len(self._values)

    def __iter__(self):
        """ Returns a (path, list) iterable over the metrics """
        return self._values.__iter__()

    def __contains__(self, path: str) -> bool:
        """
        Returns True if metric with specified path is in the snapshot,
        False otherwise.
        """
        return path in self._values

    def __eq__(self, other: object) -> bool:
        """
        Compares the snapshot with a dict of metrics or with
        another snapshot
        """
        return self._values == other

    def __repr__(self) -> str:
        return self._values.__repr__()

    def __str__(self) -> str:
        return self.pretty_print()

    def get(self, path: str, default=None):
        """
        Returns an list of metrics by path or default if there's no
        such path
        """
        return self._values.get(path, default)

    def items(self):
        """ Returns a (path, list) iterable over the metrics """
        return self._values.items()

    def keys(self):
        """ Returns an iterable over paths of metrics """
        return self._values.keys()

    def values(self):
        """ Returns an iterable over lists of metrics """
        return self._values.values()

    def value_at(
            self,
            path: str,
            labels: typing.Optional[typing.Dict] = None,
            *,
            default: typing.Optional[MetricValue] = None,
    ) -> MetricValue:
        """
        Returns a single metric value at specified path. If a dict of labels
        is provided, does en exact match of labels (i.e. {} stands for no
        labels; {'a': 'b', 'c': 'd'} matches only {'a': 'b', 'c': 'd'} or
        {'c': 'd', 'a': 'b'} but neither match {'a': 'b'} nor
        {'a': 'b', 'c': 'd', 'e': 'f'}).

        @throws AssertionError if not one metric by path
        """
        entry = self.get(path, set())
        assert (
            entry or default is not None
        ), f'No metrics found by path "{path}"'

        if labels is not None:
            entry = {x for x in entry if x.labels == labels}
            assert (
                entry or default is not None
            ), f'No metrics found by path "{path}" and labels {labels}'
            assert len(entry) <= 1, (
                f'Multiple metrics found by path "{path}" and labels {labels}:'
                f' {entry}'
            )
        else:
            assert (
                len(entry) <= 1
            ), f'Multiple metrics found by path "{path}": {entry}'

        if default is not None and not entry:
            return default
        return next(iter(entry)).value

    def metrics_at(
            self,
            path: str,
            require_labels: typing.Optional[typing.Dict] = None,
    ) -> typing.List[Metric]:
        """
        Metrics path must exactly equal the given `path`.
        A required subset of labels is specified by `require_labels`
        Example:
        require_labels={'a':'b', 'c':'d'}
        { 'a':'b', 'c':'d'} - exact match
        { 'a':'b', 'c':'d', 'e': 'f', 'h':'k'} - match
        { 'a':'x', 'c':'d'} - no match, incorrect value for label 'a'
        { 'a' : 'b'} - required label not found
        Usage:
        @code
        for m in metrics_with_labels(path='something.something.sensor',
          require_labels={ 'label1': 'value1' }):
           assert m.value > 0
        @endcode
        """
        entry = self.get(path, set())

        def _is_labels_subset(require_labels, target_labels) -> bool:
            for req_key, req_val in require_labels.items():
                if target_labels.get(req_key, None) != req_val:
                    # required label is missing or its value is different
                    return False
            return True

        if require_labels is not None:
            return list(
                filter(
                    lambda x: _is_labels_subset(
                        require_labels=require_labels, target_labels=x.labels,
                    ),
                    entry,
                ),
            )
        else:
            return list(entry)

    def has_metrics_at(
            self,
            path: str,
            require_labels: typing.Optional[typing.Dict] = None,
    ) -> bool:
        # metrics_with_labels returns list, and pythonic way to check if list
        # is empty is like this:
        return bool(self.metrics_at(path, require_labels))

    def assert_equals(
            self,
            other: typing.Mapping[str, typing.Set[Metric]],
            *,
            ignore_zeros: bool = False,
    ) -> None:
        """
        Compares the snapshot with a dict of metrics or with
        another snapshot, displaying a nice diff on mismatch
        """
        lhs = _flatten_snapshot(self, ignore_zeros=ignore_zeros)
        rhs = _flatten_snapshot(other, ignore_zeros=ignore_zeros)
        assert lhs == rhs, _diff_metric_snapshots(lhs, rhs, ignore_zeros)

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
            """ print (pretty) one metrics set - for given path """
            result = []
            for metric in sorted(mset, key=lambda x: _get_labels_tuple(x)):
                result.append(
                    '{}: {} {} {}'.format(
                        path,
                        # labels in form (key=value)
                        ','.join(
                            [
                                '({}={})'.format(k, v)
                                for k, v in _get_labels_tuple(metric)
                            ],
                        ),
                        metric._type.value,
                        metric.value,
                    ),
                )
            return result

        # list of lists [ [ string1, string2, string3],
        #                 [string4, string5, string6] ]
        data_for_every_path = [
            _iterate_over_mset(path, mset)
            for path, mset in self._values.items()
        ]
        # use itertools.chain to flatten list
        # [ string1, string2, string3, string4, string5, string6 ]
        # and join to convert it to one multiline string
        return '\n'.join(itertools.chain(*data_for_every_path))

    @staticmethod
    def from_json(json_str: str) -> 'MetricsSnapshot':
        """
        Construct MetricsSnapshot from a JSON string
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
            for path, metrics_list in json.loads(json_str).items()
        }
        return MetricsSnapshot(json_data)

    def to_json(self) -> str:
        """
        Serialize to a JSON string
        """
        return json.dumps(
            # Shuffle to disallow depending on the received metrics order.
            {
                path: random.sample(list(metrics), len(metrics))
                for path, metrics in self._values.items()
            },
            cls=_MetricsJSONEncoder,
        )


def _type_eq(lhs: MetricType, rhs: MetricType) -> bool:
    return (
        lhs == rhs
        or lhs == MetricType.UNSPECIFIED
        or rhs == MetricType.UNSPECIFIED
    )


def _get_labels_tuple(metric: Metric) -> typing.Tuple:
    """ Returns labels as a tuple of sorted items """
    return tuple(sorted(metric.labels.items()))


def _do_compute_percentile(hist: Histogram, percent: float) -> float:
    # This implementation is O(hist.count()), which is less than perfect.
    # So far, this was not a big enough pain to rewrite it.
    value_lists = [
        [bound] * bucket for (bucket, bound) in zip(hist.buckets, hist.bounds)
    ] + [[math.inf] * hist.inf]
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


def _parse_metric_value(value: typing.Any) -> MetricValue:
    if isinstance(value, dict):
        return Histogram(
            bounds=value['bounds'], buckets=value['buckets'], inf=value['inf'],
        )
    elif isinstance(value, float):
        return value
    elif isinstance(value, int):
        return value
    else:
        raise Exception(f'Failed to parse metric value from {value!r}')


_FlattenedSnapshot = typing.Set[typing.Tuple[str, Metric]]


def _flatten_snapshot(values, ignore_zeros: bool) -> _FlattenedSnapshot:
    return set(
        (path, metric)
        for path, metrics in values.items()
        for metric in metrics
        if metric.value != 0 or not ignore_zeros
    )


def _diff_metric_snapshots(
        lhs: _FlattenedSnapshot, rhs: _FlattenedSnapshot, ignore_zeros: bool,
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
