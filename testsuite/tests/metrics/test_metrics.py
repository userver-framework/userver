import math

import pytest
from pytest_userver import metrics

_ETHALON_METRICS = {
    'tcp-echo.bytes.read': {metrics.Metric(labels={}, value=334)},
    'tcp-echo.sockets.opened': {metrics.Metric(labels={}, value=1)},
    'tcp-echo.sockets.closed': {metrics.Metric(labels={'a': 'b'}, value=0)},
}


def test_metrics_captured_basic():
    values = metrics.MetricsSnapshot(_ETHALON_METRICS)
    assert values == _ETHALON_METRICS
    assert _ETHALON_METRICS == values
    assert values == metrics.MetricsSnapshot(_ETHALON_METRICS)
    assert metrics.MetricsSnapshot(_ETHALON_METRICS) == values

    assert values.value_at('tcp-echo.bytes.read') == 334
    assert values.value_at('tcp-echo.bytes.read', {}) == 334
    with pytest.raises(AssertionError):
        values.value_at('tcp-echo.bytes.read', {'label': 'value'})

    assert values.value_at('tcp-echo.sockets.opened') == 1
    assert values.value_at('tcp-echo.sockets.opened', {}) == 1
    with pytest.raises(AssertionError):
        values.value_at('tcp-echo.bytes.read', {'label': 'value'})

    assert values.get('NON_EXISTING') is None
    with pytest.raises(AssertionError):
        values.value_at('NON_EXISTING')

    assert values.value_at('tcp-echo.sockets.closed', {'a': 'b'}) == 0
    with pytest.raises(AssertionError):
        values.value_at('tcp-echo.sockets.closed', {})

    assert 'tcp-echo.sockets.opened' in values.keys()


def test_metrics_eq_ignores_empty_paths():
    values = metrics.MetricsSnapshot(_ETHALON_METRICS)

    # an extra path mapped to an empty set of metrics does not affect equality
    assert values == {**_ETHALON_METRICS, 'extra-path': set()}
    assert {**_ETHALON_METRICS, 'extra-path': set()} == values
    assert values == metrics.MetricsSnapshot({**_ETHALON_METRICS, 'extra-path': set()})
    assert metrics.MetricsSnapshot({**_ETHALON_METRICS, 'extra-path': set()}) == values

    # both sides may have their own extra empty paths at the same time
    assert metrics.MetricsSnapshot({**_ETHALON_METRICS, 'extra-path-a': set()}) == metrics.MetricsSnapshot({
        **_ETHALON_METRICS,
        'extra-path-b': set(),
    })

    # a non-empty path is never dropped, so a missing one is still a mismatch
    assert values != {k: v for k, v in _ETHALON_METRICS.items() if k != 'tcp-echo.bytes.read'}


def test_metrics_value_at_default():
    values = metrics.MetricsSnapshot({
        'tcp-echo.bytes.read': {metrics.Metric(labels={}, value=334)},
        'tcp-echo.sockets.closed': {
            metrics.Metric(labels={'a': 'b'}, value=0),
        },
        'tickets.closed': {
            metrics.Metric(labels={'a': 'b'}, value=10),
            metrics.Metric(labels={'a': 'c'}, value=20),
        },
    })

    assert values.value_at('tcp-echo.bytes.read', default=0) == 334
    assert values.value_at('tcp-echo.bytes.read', default=42) == 334
    assert values.value_at('tcp-echo.sockets.closed', {'a': 'b'}, default=42) == 0
    assert values.value_at('NON_EXISTING', default=0) == 0
    assert values.value_at('NON_EXISTING', default=42) == 42
    assert values.value_at('tcp-echo.bytes.read', {'label': 'value'}, default=0) == 0
    assert values.value_at('tcp-echo.bytes.read', {'label': 'value'}, default=42) == 42
    with pytest.raises(AssertionError, match='Multiple metrics'):
        values.value_at('tickets.closed', default=42)


def test_metrics_list():
    ethalon_list = {
        'test': {
            metrics.Metric(labels={'label': 'a'}, value=1),
            metrics.Metric(labels={'label': 'b'}, value=2),
            metrics.Metric(labels={'label2': 'a'}, value=3),
            metrics.Metric(labels={'label2': 'b'}, value=4),
            metrics.Metric(labels={'label': 'a', 'label2': 'a'}, value=5),
            metrics.Metric(labels={'label': 'b', 'label2': 'b'}, value=6),
        },
    }

    values = metrics.MetricsSnapshot(ethalon_list)
    assert values.value_at('test', labels={'label': 'a'}) == 1
    assert values.value_at('test', labels={'label2': 'b'}) == 4

    # equal to ethalon, order changed
    assert values['test'] == {
        metrics.Metric(labels={'label': 'a', 'label2': 'a'}, value=5),
        metrics.Metric(labels={'label': 'b', 'label2': 'b'}, value=6),
        metrics.Metric(labels={'label2': 'a'}, value=3),
        metrics.Metric(labels={'label2': 'b'}, value=4),
        metrics.Metric(labels={'label': 'a'}, value=1),
        metrics.Metric(labels={'label': 'b'}, value=2),
    }

    assert {
        metrics.Metric(labels={'label': 'a'}, value=1),
        metrics.Metric(labels={'label': 'a', 'label2': 'a'}, value=5),
        metrics.Metric(labels={'label': 'b', 'label2': 'b'}, value=6),
        metrics.Metric(labels={'label2': 'a'}, value=3),
        metrics.Metric(labels={'label2': 'b'}, value=4),
        metrics.Metric(labels={'label': 'b'}, value=2),
    } == values['test']

    # values mismatch
    assert {
        metrics.Metric(labels={'label': 'a'}, value=1),
        metrics.Metric(labels={'label': 'a', 'label2': 'a'}, value=5),
        metrics.Metric(labels={'label': 'b', 'label2': 'b'}, value=6),
        metrics.Metric(labels={'label2': 'a'}, value=3),
        metrics.Metric(labels={'label2': 'b'}, value=4),
        metrics.Metric(labels={'label': 'b'}, value=200),
    } != values['test']

    # different count of values
    assert values['test'] != {
        metrics.Metric(labels={'label': 'a', 'label2': 'a'}, value=5),
        metrics.Metric(labels={'label': 'b', 'label2': 'b'}, value=6),
        metrics.Metric(labels={'label2': 'a'}, value=3),
        metrics.Metric(labels={'label2': 'b'}, value=4),
        metrics.Metric(labels={'label': 'a'}, value=1),
    }

    # different label value
    assert values['test'] != {
        metrics.Metric(labels={'label': 'a', 'label2': 'a'}, value=5),
        metrics.Metric(labels={'label': 'c', 'label2': 'b'}, value=6),
        metrics.Metric(labels={'label2': 'a'}, value=3),
        metrics.Metric(labels={'label2': 'b'}, value=4),
        metrics.Metric(labels={'label': 'a'}, value=1),
        metrics.Metric(labels={'label': 'b'}, value=2),
    }

    values = values['test']
    assert metrics.Metric({'label': 'a'}, value=1) in values

    assert metrics.Metric({}, value=1) not in values
    assert metrics.Metric({'label': 'a'}, value=0) not in values
    assert metrics.Metric({'label': 'c'}, value=1) not in values


def test_metrics_list_sample():
    values = metrics.MetricsSnapshot({
        'sample': {
            metrics.Metric(labels={'label': 'a'}, value=1),
            metrics.Metric(labels={'label': 'b'}, value=2),
            metrics.Metric(labels={}, value=3),
        },
    })

    # /// [values set]
    # Checking for a particular metric
    assert metrics.Metric({}, value=3) in values['sample']

    # Comparing with a set of Metric
    assert values['sample'] == {
        metrics.Metric(labels={}, value=3),
        metrics.Metric(labels={'label': 'b'}, value=2),
        metrics.Metric(labels={'label': 'a'}, value=1),
    }
    # /// [values set]


def test_equals_ignore_zeros():
    values = metrics.MetricsSnapshot({
        'sample': {
            metrics.Metric(labels={'label': 'a'}, value=0),
            metrics.Metric(labels={'label': 'b'}, value=3),
            metrics.Metric(labels={'other_label': 'a'}, value=0),
        },
        'other_sample': {metrics.Metric(labels={'label': 'a'}, value=0)},
    })

    # equal to ethalon, old zero metrics removed, new zero metrics added
    values.assert_equals(
        {
            'sample': {
                metrics.Metric(labels={'label': 'c'}, value=0),
                metrics.Metric(labels={'label': 'b'}, value=3),
                metrics.Metric(labels={'another_label': 'a'}, value=0),
            },
            'another_sample': {metrics.Metric(labels={'label': 'a'}, value=0)},
        },
        ignore_zeros=True,
    )

    # equal to ethalon, old zero metrics removed
    values.assert_equals(
        {'sample': {metrics.Metric(labels={'label': 'b'}, value=3)}},
        ignore_zeros=True,
    )

    # not equal to ethalon, old non-zero metric changed or removed
    with pytest.raises(AssertionError):
        values.assert_equals(
            {
                'sample': {
                    metrics.Metric(labels={'label': 'b', 'x': 'x'}, value=5),
                },
            },
            ignore_zeros=True,
        )
    with pytest.raises(AssertionError):
        values.assert_equals(
            {'sample': {metrics.Metric(labels={'label': 'b'}, value=5)}},
            ignore_zeros=True,
        )
    with pytest.raises(AssertionError):
        values.assert_equals(
            {'sample': {metrics.Metric(labels={'label': 'b'}, value=0)}},
            ignore_zeros=True,
        )
    with pytest.raises(AssertionError):
        values.assert_equals({'sample': set()}, ignore_zeros=True)
    with pytest.raises(AssertionError):
        values.assert_equals({}, ignore_zeros=True)

    # not equal to ethalon, new non-zero metrics added
    with pytest.raises(AssertionError):
        values.assert_equals(
            {
                'sample': {
                    metrics.Metric(labels={'label': 'b'}, value=3),
                    metrics.Metric(labels={'label': 'c'}, value=5),
                },
            },
            ignore_zeros=True,
        )
    with pytest.raises(AssertionError):
        values.assert_equals(
            {
                'sample': {
                    metrics.Metric(labels={'label': 'b'}, value=3),
                    metrics.Metric(labels={'another_label': 'c'}, value=5),
                },
            },
            ignore_zeros=True,
        )
    with pytest.raises(AssertionError):
        values.assert_equals(
            {
                'sample': {metrics.Metric(labels={'label': 'b'}, value=3)},
                'another_sample': {
                    metrics.Metric(labels={'label': 'c'}, value=5),
                },
            },
            ignore_zeros=True,
        )


def test_without_zero_rates():
    zero_histogram = metrics.Histogram(bounds=[1, 2, 3], buckets=[0, 0, 0], inf=0)
    non_zero_histogram = metrics.Histogram(bounds=[1, 2, 3], buckets=[0, 1, 0], inf=0)

    values = metrics.MetricsSnapshot({
        'gauge': {metrics.Metric(labels={}, value=0, _type=metrics.MetricType.GAUGE)},
        'untyped-zero': {metrics.Metric(labels={}, value=0)},
        'rate': {
            metrics.Metric(labels={'state': 'zero'}, value=0, _type=metrics.MetricType.RATE),
            metrics.Metric(labels={'state': 'non-zero'}, value=5, _type=metrics.MetricType.RATE),
        },
        'hist-rate': {
            metrics.Metric(labels={'state': 'zero'}, value=zero_histogram, _type=metrics.MetricType.HIST_RATE),
            metrics.Metric(labels={'state': 'non-zero'}, value=non_zero_histogram, _type=metrics.MetricType.HIST_RATE),
        },
        'hist-rate-all-zero': {
            metrics.Metric(labels={}, value=zero_histogram, _type=metrics.MetricType.HIST_RATE),
        },
    })

    assert values.without_zero_rates() == {
        # GAUGE and untyped metrics are kept even if their value is zero
        'gauge': {metrics.Metric(labels={}, value=0, _type=metrics.MetricType.GAUGE)},
        'untyped-zero': {metrics.Metric(labels={}, value=0)},
        # only the zero-valued RATE metric is dropped
        'rate': {metrics.Metric(labels={'state': 'non-zero'}, value=5, _type=metrics.MetricType.RATE)},
        # only the all-zero histogram is dropped
        'hist-rate': {
            metrics.Metric(labels={'state': 'non-zero'}, value=non_zero_histogram, _type=metrics.MetricType.HIST_RATE),
        },
        # 'hist-rate-all-zero' path had its only metric dropped, so it disappears entirely
    }
    assert 'hist-rate-all-zero' not in values.without_zero_rates()


def test_metrics_captured_json():
    json = """{
        "tcp-echo.bytes.read": [{"labels": {}, "value": 334}],
        "tcp-echo.sockets.opened": [{"labels": {}, "value": 1}],
        "tcp-echo.sockets.closed": [{"labels": {"a":"b"}, "value": 0}]
    }"""
    values = metrics.MetricsSnapshot.from_json(json)
    assert values.value_at('tcp-echo.bytes.read') == 334
    assert values.value_at('tcp-echo.bytes.read', {}) == 334
    assert values.value_at('tcp-echo.sockets.opened') == 1
    assert values.value_at('tcp-echo.sockets.opened', {}) == 1
    assert values.value_at('tcp-echo.sockets.closed', {'a': 'b'}) == 0
    with pytest.raises(AssertionError):
        values.value_at('tcp-echo.sockets.closed', {'a': 'd'})

    assert values == _ETHALON_METRICS
    new_values = metrics.MetricsSnapshot.from_json(values.to_json())
    assert values == new_values


def test_handmade_metrics_to_json():
    values = metrics.MetricsSnapshot({
        'tcp-echo.bytes.read': {metrics.Metric(labels={}, value=1)},
        'tcp-echo.sockets.closed': {
            metrics.Metric(labels={'label': 'b'}, value=2),
        },
    })

    new_values = metrics.MetricsSnapshot.from_json(values.to_json())
    assert values == new_values

    json = """{
        "tcp-echo.bytes.read": [{"labels": {}, "value": 1}],
        "tcp-echo.sockets.closed": [{"labels": {"label":"b"}, "value": 2}]
    }"""
    assert values == metrics.MetricsSnapshot.from_json(json)


def test_init_common_prefix():
    values = metrics.MetricsSnapshot(
        {'bytes.read': {metrics.Metric({}, 334)}, 'sockets.closed': {metrics.Metric({}, 2)}},
        common_prefix='tcp-echo',
    )
    assert values == metrics.MetricsSnapshot({
        'tcp-echo.bytes.read': {metrics.Metric({}, 334)},
        'tcp-echo.sockets.closed': {metrics.Metric({}, 2)},
    })


def test_init_common_labels():
    values = metrics.MetricsSnapshot(
        {'a': {metrics.Metric({}, 1)}, 'b': {metrics.Metric({}, 2)}},
        common_labels={'zone': 'paris'},
    )
    assert values == metrics.MetricsSnapshot({
        'a': {metrics.Metric({'zone': 'paris'}, 1)},
        'b': {metrics.Metric({'zone': 'paris'}, 2)},
    })


def test_init_common_labels_merged_with_own_labels():
    values = metrics.MetricsSnapshot(
        {'a': {metrics.Metric({'x': 'foo'}, 1)}},
        common_labels={'zone': 'paris'},
    )
    assert values == metrics.MetricsSnapshot({
        'a': {metrics.Metric({'zone': 'paris', 'x': 'foo'}, 1)},
    })


def test_init_common_labels_overridden_by_own_labels():
    values = metrics.MetricsSnapshot(
        {'a': {metrics.Metric({'zone': 'london'}, 1)}},
        common_labels={'zone': 'paris'},
    )
    assert values == metrics.MetricsSnapshot({
        'a': {metrics.Metric({'zone': 'london'}, 1)},
    })


def test_init_common_prefix_and_labels():
    values = metrics.MetricsSnapshot(
        {'a': {metrics.Metric({'x': 'foo'}, 1)}},
        common_prefix='root',
        common_labels={'zone': 'paris'},
    )
    assert values == metrics.MetricsSnapshot({
        'root.a': {metrics.Metric({'zone': 'paris', 'x': 'foo'}, 1)},
    })


def test_layered_dict_no_labels():
    values = metrics.MetricsSnapshot.from_layered_dict({
        'tcp-echo.bytes.read': 334,
    })
    assert values == metrics.MetricsSnapshot({
        'tcp-echo.bytes.read': {metrics.Metric({}, 334)},
    })


def test_layered_dict_single_label():
    values = metrics.MetricsSnapshot.from_layered_dict({
        'a': {'x = foo': 1, 'x = bar': 2},
    })
    assert values == metrics.MetricsSnapshot({
        'a': {metrics.Metric({'x': 'foo'}, 1), metrics.Metric({'x': 'bar'}, 2)},
    })


def test_layered_dict_multiple_labels():
    # Same shape as a "versus"/"entity_type" match-statistic metric: one
    # level of nesting per label, plus a leaf ('versus = total') that skips
    # the inner label entirely.
    values = metrics.MetricsSnapshot.from_layered_dict({
        'match-statistic': {
            'versus = found': {'entity_type = udid': 4, 'entity_type = park': 2},
            'versus = not_found_percent': {'entity_type = udid': 0.0, 'entity_type = park': 50.0},
            'versus = total': 4,
        },
    })
    assert values == metrics.MetricsSnapshot({
        'match-statistic': {
            metrics.Metric({'versus': 'found', 'entity_type': 'udid'}, 4),
            metrics.Metric({'versus': 'found', 'entity_type': 'park'}, 2),
            metrics.Metric({'versus': 'not_found_percent', 'entity_type': 'udid'}, 0.0),
            metrics.Metric({'versus': 'not_found_percent', 'entity_type': 'park'}, 50.0),
            metrics.Metric({'versus': 'total'}, 4),
        },
    })


def test_layered_dict_label_key_without_equals_fails():
    # Below the top level (paths), every key must be a 'name = value' label.
    with pytest.raises(AssertionError):
        metrics.MetricsSnapshot.from_layered_dict({'a': {'b': 1}})


@pytest.mark.parametrize(
    'key',
    [
        'x=foo',
        'x =foo',
        'x= foo',
        'x  = foo',
        'x =  foo',
    ],
)
def test_layered_dict_label_key_with_wrong_spacing_fails(key):
    # Exactly one space is required on each side of '='.
    with pytest.raises(AssertionError):
        metrics.MetricsSnapshot.from_layered_dict({'a': {key: 1}})


def test_layered_dict_histogram_leaf():
    # A dict with 'bounds' and 'buckets' keys is parsed as a Histogram, same as `from_dict`.
    values = metrics.MetricsSnapshot.from_layered_dict({
        'a': {'x = foo': {'bounds': [10, 20], 'buckets': [1, 2], 'inf': 3}},
    })
    assert values == metrics.MetricsSnapshot({
        'a': {metrics.Metric({'x': 'foo'}, metrics.Histogram(bounds=[10, 20], buckets=[1, 2], inf=3))},
    })


def test_layered_dict_matches_from_dict():
    nested = metrics.MetricsSnapshot.from_layered_dict({
        'a': {'x = foo': 1, 'x = bar': 2},
        'b': 3,
    })
    flat = metrics.MetricsSnapshot.from_dict({
        'a': [
            {'labels': {'x': 'foo'}, 'value': 1},
            {'labels': {'x': 'bar'}, 'value': 2},
        ],
        'b': [{'labels': {}, 'value': 3}],
    })
    assert nested == flat


def test_layered_dict_common_prefix():
    values = metrics.MetricsSnapshot.from_layered_dict(
        {'bytes.read': 334, 'sockets.closed': 2},
        common_prefix='tcp-echo',
    )
    assert values == metrics.MetricsSnapshot({
        'tcp-echo.bytes.read': {metrics.Metric({}, 334)},
        'tcp-echo.sockets.closed': {metrics.Metric({}, 2)},
    })


def test_layered_dict_common_prefix_with_labels():
    values = metrics.MetricsSnapshot.from_layered_dict(
        {'a': {'x = foo': 1, 'x = bar': 2}},
        common_prefix='root',
    )
    assert values == metrics.MetricsSnapshot({
        'root.a': {metrics.Metric({'x': 'foo'}, 1), metrics.Metric({'x': 'bar'}, 2)},
    })


def test_layered_dict_common_labels():
    values = metrics.MetricsSnapshot.from_layered_dict(
        {'a': 1, 'b': 2},
        common_labels={'zone': 'paris'},
    )
    assert values == metrics.MetricsSnapshot({
        'a': {metrics.Metric({'zone': 'paris'}, 1)},
        'b': {metrics.Metric({'zone': 'paris'}, 2)},
    })


def test_layered_dict_common_labels_with_nested_labels():
    values = metrics.MetricsSnapshot.from_layered_dict(
        {'a': {'x = foo': 1}},
        common_labels={'zone': 'paris'},
    )
    assert values == metrics.MetricsSnapshot({
        'a': {metrics.Metric({'zone': 'paris', 'x': 'foo'}, 1)},
    })


def test_layered_dict_common_prefix_and_labels():
    values = metrics.MetricsSnapshot.from_layered_dict(
        {'a': {'x = foo': 1}},
        common_prefix='root',
        common_labels={'zone': 'paris'},
    )
    assert values == metrics.MetricsSnapshot({
        'root.a': {metrics.Metric({'zone': 'paris', 'x': 'foo'}, 1)},
    })


def test_histogram():
    # /// [histogram]
    histogram = metrics.Histogram(
        bounds=[10, 20, 30],
        buckets=[1, 3, 4],
        inf=3,
    )
    assert histogram.count() == 11
    assert histogram.percentile(0.6) == 30
    # /// [histogram]


def test_histogram_percentile():
    histogram = metrics.Histogram(
        bounds=[10, 20, 30],
        buckets=[1, 3, 4],
        inf=3,
    )
    assert histogram.percentile(0) == 10
    assert histogram.percentile(0.05) == 15
    assert histogram.percentile(0.1) == 20
    assert histogram.percentile(0.2) == 20
    assert histogram.percentile(0.3) == 20
    assert histogram.percentile(0.35) == 25
    assert histogram.percentile(0.4) == 30
    assert histogram.percentile(0.5) == 30
    assert histogram.percentile(0.6) == 30
    assert histogram.percentile(0.7) == 30
    assert histogram.percentile(0.71) == math.inf
    assert histogram.percentile(0.8) == math.inf
    assert histogram.percentile(0.9) == math.inf
    assert histogram.percentile(1) == math.inf


def test_compare_visitor_applies_only_when_metrics_snapshot_involved():
    left_metric = metrics.Metric(labels={'a': 'b'}, value=1)
    right_metric = metrics.Metric(labels={'a': 'b'}, value=2)

    # Neither side is a MetricsSnapshot: our visitor must not kick in, so
    # the mismatching metrics are shown via Metric's own repr, not our
    # "path: (labels) type value" format.
    with pytest.raises(AssertionError) as exc_info:
        assert {'p': {left_metric}} == {'p': {right_metric}}
    plain_message = str(exc_info.value)
    assert 'Metric(labels=' in plain_message
    assert 'p: (a=b)' not in plain_message


@pytest.mark.parametrize('wrap_left, wrap_right', [(True, False), (False, True), (True, True)])
def test_compare_visitor_applies_regardless_of_which_side_is_a_snapshot(wrap_left, wrap_right):
    left_data = {'p': {metrics.Metric(labels={'a': 'b'}, value=1)}}
    right_data = {'p': {metrics.Metric(labels={'a': 'b'}, value=2)}}
    left = metrics.MetricsSnapshot(left_data) if wrap_left else left_data
    right = metrics.MetricsSnapshot(right_data) if wrap_right else right_data

    with pytest.raises(AssertionError) as exc_info:
        assert left == right
    assert 'p: (a=b)' in str(exc_info.value)


def test_compare_visitor_resolves_unspecified_type_from_other_side():
    # 'matches' has an explicit RATE type on the right and none on the
    # left: it must be treated as a full match (not shown in the diff at
    # all), because the UNSPECIFIED type is resolved from the matching
    # metric on the other side. 'sentinel' differs for real, just to force
    # the assertion (and thus the diff) to happen.
    left = metrics.MetricsSnapshot({
        'matches': {metrics.Metric(labels={'a': 'b'}, value=5)},
        'sentinel': {metrics.Metric(labels={}, value=1)},
    })
    right = {
        'matches': {metrics.Metric(labels={'a': 'b'}, value=5, _type=metrics.MetricType.RATE)},
        'sentinel': {metrics.Metric(labels={}, value=2)},
    }

    with pytest.raises(AssertionError) as exc_info:
        assert left == right

    our_explanation = str(exc_info.value).split('pytest default:')[0]
    assert 'matches' not in our_explanation
    assert 'sentinel' in our_explanation


def test_compare_visitor_keeps_type_unspecified_when_other_side_has_no_match():
    left = metrics.MetricsSnapshot({'only-left': {metrics.Metric(labels={'a': 'b'}, value=5)}})
    right = {}

    with pytest.raises(AssertionError) as exc_info:
        assert left == right

    our_explanation = str(exc_info.value).split('pytest default:')[0]
    assert "extra items on the left: 'only-left: (a=b) UNSPECIFIED 5'" in our_explanation


def test_compare_visitor_message_on_failing_assert():
    left = metrics.MetricsSnapshot({
        'a': {metrics.Metric(labels={}, value=1, _type=metrics.MetricType.GAUGE)},
        # 'b' has no explicit type here, but 'right' below has RATE with the
        # same labels and value: they must be treated as a full match.
        'b': {metrics.Metric(labels={'x': '1'}, value=2)},
    })
    right = {
        'a': {metrics.Metric(labels={}, value=5, _type=metrics.MetricType.GAUGE)},
        'b': {metrics.Metric(labels={'x': '1'}, value=2, _type=metrics.MetricType.RATE)},
    }

    with pytest.raises(AssertionError) as exc_info:
        assert left == right

    message = str(exc_info.value)
    # Our contributed explanation comes before pytest's own "pytest default:"
    # section (which dumps the *whole* set for context, matched items
    # included, and thus is expected to still mention 'b').
    our_explanation = message.split('pytest default:')[0]

    assert "extra items on the left: 'a: GAUGE 1'" in our_explanation
    assert "extra items on the right: 'a: GAUGE 5'" in our_explanation
    # 'b' matched exactly (UNSPECIFIED borrowed RATE from the other side),
    # so it must not show up as an actual diff.
    assert 'b:' not in our_explanation
