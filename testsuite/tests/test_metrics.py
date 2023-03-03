import pytest
from pytest_userver import metrics  # pylint: disable=import-error


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


def test_metrics_value_at_default():
    values = metrics.MetricsSnapshot(
        {
            'tcp-echo.bytes.read': {metrics.Metric(labels={}, value=334)},
            'tcp-echo.sockets.closed': {
                metrics.Metric(labels={'a': 'b'}, value=0),
            },
            'tickets.closed': {
                metrics.Metric(labels={'a': 'b'}, value=10),
                metrics.Metric(labels={'a': 'c'}, value=20),
            },
        },
    )

    assert values.value_at('tcp-echo.bytes.read', default=0) == 334
    assert values.value_at('tcp-echo.bytes.read', default=42) == 334
    assert (
        values.value_at('tcp-echo.sockets.closed', {'a': 'b'}, default=42) == 0
    )
    assert values.value_at('NON_EXISTING', default=0) == 0
    assert values.value_at('NON_EXISTING', default=42) == 42
    assert (
        values.value_at('tcp-echo.bytes.read', {'label': 'value'}, default=0)
        == 0
    )
    assert (
        values.value_at('tcp-echo.bytes.read', {'label': 'value'}, default=42)
        == 42
    )
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
    values = metrics.MetricsSnapshot(
        {
            'sample': {
                metrics.Metric(labels={'label': 'a'}, value=1),
                metrics.Metric(labels={'label': 'b'}, value=2),
                metrics.Metric(labels={}, value=3),
            },
        },
    )

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
    values = metrics.MetricsSnapshot(
        {
            'sample': {
                metrics.Metric(labels={'label': 'a'}, value=0),
                metrics.Metric(labels={'label': 'b'}, value=3),
                metrics.Metric(labels={'other_label': 'a'}, value=0),
            },
            'other_sample': {metrics.Metric(labels={'label': 'a'}, value=0)},
        },
    )

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
    values = metrics.MetricsSnapshot(
        {
            'tcp-echo.bytes.read': {metrics.Metric(labels={}, value=1)},
            'tcp-echo.sockets.closed': {
                metrics.Metric(labels={'label': 'b'}, value=2),
            },
        },
    )

    new_values = metrics.MetricsSnapshot.from_json(values.to_json())
    assert values == new_values

    json = """{
        "tcp-echo.bytes.read": [{"labels": {}, "value": 1}],
        "tcp-echo.sockets.closed": [{"labels": {"label":"b"}, "value": 2}]
    }"""
    assert values == metrics.MetricsSnapshot.from_json(json)
