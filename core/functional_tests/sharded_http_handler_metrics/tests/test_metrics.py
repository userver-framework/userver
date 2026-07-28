import pytest
import pytest_userver.metrics

REQUIRED_LABELS = {
    'http_handler': 'handler-very-important-product',
    'version': '2',
    'http_path': '/very-important-product',
}
SPECIAL_LABELS = {'product_slice': 'abc'}
METRICS_PREFIX = 'http.handler'


@pytest.mark.parametrize('set_sharded_metrics', [False, True])
async def test_metrics(service_client, monitor_client, set_sharded_metrics):
    async with monitor_client.metrics_diff(prefix=METRICS_PREFIX, labels=REQUIRED_LABELS) as differ:
        response = await service_client.get(
            '/very-important-product', params={'product_slice': 'abc'} if set_sharded_metrics else {}
        )
    assert response.status_code == 200

    # `differ` was constructed with `prefix=METRICS_PREFIX, labels=REQUIRED_LABELS` and is sliced
    # by default, so `differ.diff` already has `METRICS_PREFIX` stripped from paths and
    # `REQUIRED_LABELS` auto-merged into any `labels=`/`require_labels=` passed to it; below,
    # `Metric(...)` objects still need their *own* full label set including `REQUIRED_LABELS`,
    # since `Metric.labels` isn't affected by slicing (only lookups are).
    special_assert = {}
    if set_sharded_metrics:
        special_labels = REQUIRED_LABELS | SPECIAL_LABELS
        special_assert = {
            'product_sliced.rps': {pytest_userver.metrics.Metric(value=1, labels=special_labels)},
            'product_sliced.cancelled-by-deadline': {pytest_userver.metrics.Metric(value=0, labels=special_labels)},
            'product_sliced.deadline-received': {pytest_userver.metrics.Metric(value=0, labels=special_labels)},
            'product_sliced.in-flight': {pytest_userver.metrics.Metric(value=0, labels=special_labels)},
            'product_sliced.rate-limit-reached': {pytest_userver.metrics.Metric(value=0, labels=special_labels)},
            'product_sliced.too-many-requests-in-flight': {
                pytest_userver.metrics.Metric(value=0, labels=special_labels)
            },
            'product_sliced.reply-codes': {
                pytest_userver.metrics.Metric(value=1, labels={'http_code': '200'} | special_labels),
                pytest_userver.metrics.Metric(value=0, labels={'http_code': '300'} | special_labels),
                pytest_userver.metrics.Metric(value=0, labels={'http_code': '500'} | special_labels),
                pytest_userver.metrics.Metric(value=0, labels={'http_code': '501'} | special_labels),
            },
            'product_sliced.timings': differ.diff[
                'product_sliced.timings'  # We do not bother ourselves asserting timings. Rps is enough
            ],
        }

    differ.diff.assert_equals(
        {
            'rps': {pytest_userver.metrics.Metric(value=1, labels=REQUIRED_LABELS)},
            'cancelled-by-deadline': {pytest_userver.metrics.Metric(value=0, labels=REQUIRED_LABELS)},
            'deadline-received': {pytest_userver.metrics.Metric(value=0, labels=REQUIRED_LABELS)},
            'in-flight': {pytest_userver.metrics.Metric(value=0, labels=REQUIRED_LABELS)},
            'rate-limit-reached': {pytest_userver.metrics.Metric(value=0, labels=REQUIRED_LABELS)},
            'too-many-requests-in-flight': {pytest_userver.metrics.Metric(value=0, labels=REQUIRED_LABELS)},
            'reply-codes': {
                pytest_userver.metrics.Metric(value=1, labels={'http_code': '200'} | REQUIRED_LABELS),
                pytest_userver.metrics.Metric(value=0, labels={'http_code': '300'} | REQUIRED_LABELS),
                pytest_userver.metrics.Metric(value=0, labels={'http_code': '500'} | REQUIRED_LABELS),
                pytest_userver.metrics.Metric(value=0, labels={'http_code': '501'} | REQUIRED_LABELS),
            },
            'timings': differ.diff[
                'timings'  # We do not bother ourselves asserting timings. Rps is enough
            ],
        }
        | special_assert
    )
