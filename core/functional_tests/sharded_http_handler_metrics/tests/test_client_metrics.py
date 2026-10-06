import pytest
import pytest_userver.metrics

SPECIAL_LABELS = {'product_slice': 'abc'}
METRICS_PREFIX = 'httpclient'
HTTP_ERRORS = [
    'ok',
    'host-resolution-failed',
    'socket-error',
    'timeout',
    'ssl-error',
    'too-many-redirects',
    'cancelled',
    'unknown-error',
]


def _errors(labels):
    return {
        pytest_userver.metrics.Metric(value=int(http_error == 'ok'), labels={'http_error': http_error} | labels)
        for http_error in HTTP_ERRORS
    }


def _reply_statuses(labels):
    return {
        pytest_userver.metrics.Metric(value=1, labels={'http_code': '200'} | labels),
        pytest_userver.metrics.Metric(value=0, labels={'http_code': '300'} | labels),
        pytest_userver.metrics.Metric(value=0, labels={'http_code': '500'} | labels),
        pytest_userver.metrics.Metric(value=0, labels={'http_code': '501'} | labels),
    }


@pytest.mark.parametrize('set_sharded_metrics', [False, True])
async def test_client_metrics(service_client, monitor_client, mockserver, set_sharded_metrics):
    @mockserver.handler('/upstream')
    def _upstream(request):
        return mockserver.make_response()

    required_labels = {'http_destination': mockserver.url('upstream'), 'version': '2'}

    async with monitor_client.metrics_diff(prefix=METRICS_PREFIX, labels=required_labels) as differ:
        response = await service_client.get(
            '/very-important-product-upstream', params=SPECIAL_LABELS if set_sharded_metrics else {}
        )
    assert response.status_code == 200

    # `differ` is sliced by `prefix=METRICS_PREFIX, labels=required_labels`, see test_metrics.py for details.
    special_assert = {}
    if set_sharded_metrics:
        special_labels = required_labels | SPECIAL_LABELS
        special_assert = {
            'product_sliced.errors': _errors(special_labels),
            'product_sliced.reply-statuses': _reply_statuses(special_labels),
            'product_sliced.timings': differ.diff[
                'product_sliced.timings'  # We do not bother ourselves asserting timings. Errors are enough
            ],
        }

    differ.diff.assert_equals(
        {
            'errors': _errors(required_labels),
            'reply-statuses': _reply_statuses(required_labels),
            'retries': {pytest_userver.metrics.Metric(value=0, labels=required_labels)},
            'pending-requests': {pytest_userver.metrics.Metric(value=0, labels=required_labels)},
            'timeout-updated-by-deadline': {pytest_userver.metrics.Metric(value=0, labels=required_labels)},
            'cancelled-by-deadline': {pytest_userver.metrics.Metric(value=0, labels=required_labels)},
            'sockets.open': differ.diff[
                'sockets.open'  # Depends on whether the connection is reused from the previous test
            ],
            'timings': differ.diff[
                'timings'  # We do not bother ourselves asserting timings. Errors are enough
            ],
        }
        | special_assert
    )
