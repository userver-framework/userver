import pytest
import pytest_userver.client
import pytest_userver.metrics
from pytest_userver.metrics import Metric


def _make_differ(
    path: str | None = None,
    prefix: str | None = None,
    labels: dict[str, str] | None = None,
    sliced: bool = True,
    diff_gauge: bool = False,
) -> pytest_userver.client.MetricsDiffer:
    # Note: private API, do not construct MetricsDiffer like this!
    return pytest_userver.client.MetricsDiffer(
        _client=None,  # type: ignore
        _path=path,
        _prefix=prefix,
        _labels=labels,
        _sliced=sliced,
        _diff_gauge=diff_gauge,
    )


def test_not_sliced():
    differ = _make_differ(
        prefix='foo.bar',
        labels={'bar': 'qux'},
        sliced=False,
        diff_gauge=True,
    )

    differ.baseline = pytest_userver.metrics.MetricsSnapshot({
        'foo.bar.baz': {
            Metric(
                {'bar': 'qux', 'state': 'keep'},
                10,
                _type=pytest_userver.metrics.MetricType.GAUGE,
            ),
            Metric(
                {'bar': 'qux', 'state': 'remove'},
                5,
                _type=pytest_userver.metrics.MetricType.GAUGE,
            ),
        },
    })

    # 'differ' will compute 'diff' at the assignment.
    differ.current = pytest_userver.metrics.MetricsSnapshot({
        'foo.bar.baz': {
            Metric(
                {'bar': 'qux', 'state': 'keep'},
                15,
                _type=pytest_userver.metrics.MetricType.GAUGE,
            ),
            Metric(
                {'bar': 'qux', 'state': 'add'},
                15,
                _type=pytest_userver.metrics.MetricType.GAUGE,
            ),
        },
    })

    differ.diff.assert_equals({
        'foo.bar.baz': {
            Metric({'bar': 'qux', 'state': 'keep'}, 5),
            Metric({'bar': 'qux', 'state': 'add'}, 15),
        },
    })

    # value_at() is equivalent to differ.diff.value_at(): it uses the full path and the
    # exact labels as-is, without prepending 'foo.bar' or merging in {'bar': 'qux'}.
    assert differ.value_at('foo.bar.baz', {'bar': 'qux', 'state': 'keep'}) == 5
    assert differ.value_at('foo.bar.baz', {'bar': 'qux', 'state': 'add'}) == 15
    assert differ.value_at('foo.bar.baz', {'bar': 'qux', 'state': 'remove'}, default=0) == 0

    with pytest.raises(AssertionError):
        differ.value_at('foo.bar.baz', {'bar': 'qux', 'state': 'remove'})
    with pytest.raises(AssertionError):
        differ.value_at('foo.bar.baz', {})
    with pytest.raises(AssertionError):
        differ.value_at('foo.bar.baz')
    with pytest.raises(AssertionError):
        differ.value_at('nonexistent', {'bar': 'qux', 'state': 'remove'})


def test_sliced_defaults_to_unsliced_without_path_or_prefix():
    # Regression test: `sliced=True` is the default, but there is nothing to slice by
    # when neither `path` nor `prefix` is given (or `prefix=''`), so the differ must
    # behave as if `sliced=False` and keep full metric paths untouched.
    for differ in (
        _make_differ(),
        _make_differ(prefix=''),
        _make_differ(path=None, prefix=None),
    ):
        differ.baseline = pytest_userver.metrics.MetricsSnapshot({
            'foo.bar.baz': {
                Metric({'state': 'keep'}, 10, pytest_userver.metrics.MetricType.RATE),
            },
        })
        differ.current = pytest_userver.metrics.MetricsSnapshot({
            'foo.bar.baz': {
                Metric({'state': 'keep'}, 15, pytest_userver.metrics.MetricType.RATE),
            },
        })

        assert differ.value_at('foo.bar.baz', {'state': 'keep'}) == 5
        with pytest.raises(AssertionError):
            differ.value_at('baz', {'state': 'keep'})


def test_sliced():
    differ = _make_differ(
        prefix='foo.bar',
        labels={'bar': 'qux'},
        diff_gauge=True,
    )

    # With sliced=True, baseline/current are expected to already have the
    # 'foo.bar' prefix stripped from their paths (as `fetch()` would produce).
    differ.baseline = pytest_userver.metrics.MetricsSnapshot({
        'baz': {
            Metric({'bar': 'qux', 'state': 'keep'}, 10, pytest_userver.metrics.MetricType.GAUGE),
            Metric({'bar': 'qux', 'state': 'remove'}, 5, pytest_userver.metrics.MetricType.GAUGE),
        },
    })

    differ.current = pytest_userver.metrics.MetricsSnapshot({
        'baz': {
            Metric({'bar': 'qux', 'state': 'keep'}, 15, pytest_userver.metrics.MetricType.GAUGE),
            Metric({'bar': 'qux', 'state': 'add'}, 15, pytest_userver.metrics.MetricType.GAUGE),
        },
    })

    differ.diff.assert_equals({
        'baz': {
            Metric({'bar': 'qux', 'state': 'keep'}, 5),
            Metric({'bar': 'qux', 'state': 'add'}, 15),
        },
    })

    # subpath and add_labels are used as-is, without prepending 'foo.bar' or
    # merging in {'bar': 'qux'}.
    assert differ.value_at('baz', {'bar': 'qux', 'state': 'keep'}) == 5
    assert differ.value_at('baz', {'bar': 'qux', 'state': 'add'}) == 15
    assert differ.value_at('baz', {'bar': 'qux', 'state': 'remove'}, default=0) == 0

    with pytest.raises(AssertionError):
        differ.value_at('baz', {'bar': 'qux', 'state': 'remove'})
    with pytest.raises(AssertionError):
        differ.value_at('nonexistent', {'bar': 'qux', 'state': 'remove'})


def test_rate():
    differ = _make_differ(
        prefix='foo.bar',
        labels={'bar': 'qux'},
        sliced=False,
        diff_gauge=False,
    )

    differ.baseline = pytest_userver.metrics.MetricsSnapshot({
        'foo.bar.baz': {
            Metric(
                {'bar': 'qux', 'state': 'gauge'},
                10,
                pytest_userver.metrics.MetricType.GAUGE,
            ),
            Metric(
                {'bar': 'qux', 'state': 'rate'},
                5,
                pytest_userver.metrics.MetricType.RATE,
            ),
            Metric(
                {'bar': 'qux', 'state': 'hist-rate'},
                pytest_userver.metrics.Histogram(bounds=[1, 2, 3], buckets=[3, 0, 1], inf=2),
                pytest_userver.metrics.MetricType.HIST_RATE,
            ),
        },
    })

    differ.current = pytest_userver.metrics.MetricsSnapshot({
        'foo.bar.baz': {
            Metric(
                {'bar': 'qux', 'state': 'gauge'},
                15,
                pytest_userver.metrics.MetricType.GAUGE,
            ),
            Metric(
                {'bar': 'qux', 'state': 'rate'},
                15,
                pytest_userver.metrics.MetricType.RATE,
            ),
            Metric(
                {'bar': 'qux', 'state': 'hist-rate'},
                pytest_userver.metrics.Histogram(bounds=[1, 2, 3], buckets=[3, 4, 5], inf=5),
                pytest_userver.metrics.MetricType.HIST_RATE,
            ),
        },
    })

    differ.diff.assert_equals({
        'foo.bar.baz': {
            # The GAUGE metric should just be taken from `current`.
            Metric(
                {'bar': 'qux', 'state': 'gauge'},
                15,
                pytest_userver.metrics.MetricType.GAUGE,
            ),
            # For the RATE metric, diff should be taken.
            Metric(
                {'bar': 'qux', 'state': 'rate'},
                10,
                pytest_userver.metrics.MetricType.RATE,
            ),
            # For the HIST_RATE metric, diff should be taken per bucket.
            Metric(
                {'bar': 'qux', 'state': 'hist-rate'},
                pytest_userver.metrics.Histogram(bounds=[1, 2, 3], buckets=[0, 4, 4], inf=3),
                pytest_userver.metrics.MetricType.HIST_RATE,
            ),
        },
    })


def test_type_mismatch():
    baseline = pytest_userver.metrics.MetricsSnapshot({
        'foo.bar': {
            Metric({'bar': 'qux'}, 10, pytest_userver.metrics.MetricType.GAUGE),
        },
    })

    current = pytest_userver.metrics.MetricsSnapshot({
        'foo.bar': {
            Metric({'bar': 'qux'}, 15, pytest_userver.metrics.MetricType.RATE),
        },
    })

    differ = _make_differ(
        prefix='foo.bar',
        labels={'bar': 'qux'},
        sliced=False,
        diff_gauge=False,
    )
    with pytest.raises(AssertionError):
        differ.baseline = baseline
        differ.current = current
        _ = differ.diff


def test_type_unspecified():
    baseline = pytest_userver.metrics.MetricsSnapshot({
        'foo.bar': {
            Metric({'bar': 'qux'}, 10, pytest_userver.metrics.MetricType.RATE),
        },
    })

    current = pytest_userver.metrics.MetricsSnapshot({
        'foo.bar': {Metric({'bar': 'qux'}, 15)},
    })

    differ = _make_differ(
        prefix='foo.bar',
        labels={'bar': 'qux'},
        sliced=False,
        diff_gauge=False,
    )
    with pytest.raises(AssertionError):
        differ.baseline = baseline
        differ.current = current
        _ = differ.diff
