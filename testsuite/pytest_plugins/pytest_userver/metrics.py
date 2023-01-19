"""
Python module that provides helpers for functional testing of metrics with
testsuite; see @ref md_en_userver_functional_testing for an introduction.

@ingroup userver_testsuite
"""

import dataclasses
import json
import typing


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
    value: int

    def __hash__(self) -> int:
        return hash(self.get_labels_tuple())

    def get_labels_tuple(self) -> typing.Tuple:
        """ Returns labels as a tuple of sorted items """
        return tuple(sorted(self.labels.items()))


class _MetricsJSONEncoder(json.JSONEncoder):
    def default(self, o):  # pylint: disable=method-hidden
        if dataclasses.is_dataclass(o):
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
            self, path: str, labels: typing.Optional[typing.Dict] = None,
    ) -> float:
        """
        Returns a single metric value at specified path. If a dict of labels
        id provided, does en exact match of labels (i.e. {} stands for no
        labels; {'a': 'b', 'c': 'd'} matches only {'a': 'b', 'c': 'd'} or
        {'c': 'd', 'a': 'b'} but neither match {'a': 'b'} nor
        {'a': 'b', 'c': 'd', 'e': 'f'}).

        @throws AssertionError if not one metric by path
        """
        entry = self[path]
        assert entry, f'No metrics found by path "{path}"'
        if labels is not None:
            entry = {x for x in entry if x.labels == labels}
            assert (
                entry
            ), f'No metrics found by path "{path}" and labels {labels}'
            assert len(entry) == 1, (
                f'Multiple metrics found by path "{path}" and labels {labels}:'
                f' {entry}'
            )
        else:
            assert (
                len(entry) == 1
            ), f'Multiple metrics found by path "{path}": {entry}'

        return next(iter(entry)).value

    @staticmethod
    def from_json(json_str: str) -> 'MetricsSnapshot':
        """
        Construct MetricsSnapshot from a JSON string
        """
        json_data = {
            str(path): {
                Metric(labels=element['labels'], value=element['value'])
                for element in metrics_list
            }
            for path, metrics_list in json.loads(json_str).items()
        }
        return MetricsSnapshot(json_data)

    def to_json(self) -> str:
        """
        Serialize to a JSON string
        """
        return json.dumps(self._values, cls=_MetricsJSONEncoder)
