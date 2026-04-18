import pytest
import pytest_userver.client
import pytest_userver.dynconf


async def _make_success_request(service_client: pytest_userver.client.Client) -> None:
    response = await service_client.post(
        'ydb/select-rows',
        json={
            'service': 'srv',
            'channels': [1, 2, 3],
            'created': '2019-10-30T11:20:00+00:00',
        },
    )
    assert response.status_code == 200


@pytest.mark.parametrize('requests_count', [1, 10])
async def test_retry_budget(
    dynamic_config: pytest_userver.dynconf.DynamicConfig,
    service_client: pytest_userver.client.Client,
    monitor_client: pytest_userver.client.ClientMonitor,
    requests_count: int,
) -> None:
    # The default value of token-ratio is 0.1. We set the value to 1 and expect
    # that the 'approx_token_count' will be increased by 1 after each request.
    max_token_count = 150
    dynamic_config.set(
        YDB_RETRY_BUDGET={
            'sampledb': {
                'max-tokens': max_token_count,
                'token-ratio': 1,
                'enabled': True,
            },
        },
    )
    await service_client.update_server_state()

    async with monitor_client.metrics_diff(prefix='ydb.retry_budget') as differ:
        for _ in range(requests_count):
            await _make_success_request(service_client)

    assert differ.value_at('account_ok') == requests_count
    assert differ.value_at('account_fail') == 0
    assert differ.current.value_at('ydb.retry_budget.approx_token_count') == (
        differ.baseline.value_at('ydb.retry_budget.approx_token_count') + requests_count
    )
    assert differ.current.value_at('ydb.retry_budget.max_token_count') == max_token_count


async def test_retry_budget_success_limit(
    dynamic_config: pytest_userver.dynconf.DynamicConfig,
    service_client: pytest_userver.client.Client,
    monitor_client: pytest_userver.client.ClientMonitor,
) -> None:
    async with monitor_client.metrics_diff(prefix='ydb.retry_budget') as differ:
        current_approx_token_count = differ.baseline.value_at('ydb.retry_budget.approx_token_count')

        dynamic_config.set(
            YDB_RETRY_BUDGET={
                'sampledb': {
                    'max-tokens': current_approx_token_count,
                    'token-ratio': 1,
                    'enabled': True,
                },
            },
        )
        await service_client.update_server_state()

        # We can't increase the approx_token_count more than the max-tokens, so the approx_token_count won't change.
        requests_count = 10
        for _ in range(requests_count):
            await _make_success_request(service_client)

    assert differ.value_at('account_ok') == requests_count
    assert differ.value_at('account_fail') == 0
    assert differ.current.value_at('ydb.retry_budget.approx_token_count') == current_approx_token_count
    assert differ.current.value_at('ydb.retry_budget.max_token_count') == current_approx_token_count
