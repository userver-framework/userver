async def _check_metrics(
        monitor_client,
        account_ok,
        account_fail,
        approx_token_count,
        max_token_count,
):
    metrics = await monitor_client.metrics(prefix='ydb.retry_budget')
    assert len(metrics) == 4
    assert account_fail == metrics.value_at('ydb.retry_budget.account_fail')
    assert account_ok == metrics.value_at('ydb.retry_budget.account_ok')
    assert approx_token_count == metrics.value_at(
        'ydb.retry_budget.approx_token_count',
    )
    assert max_token_count == metrics.value_at(
        'ydb.retry_budget.max_token_count',
    )


async def _make_success_request(service_client):
    response = await service_client.post(
        'ydb/select-rows',
        json={
            'service': 'srv',
            'channels': [1, 2, 3],
            'created': '2019-10-30T11:20:00+00:00',
        },
    )
    assert response.status_code == 200


async def _get_current_metric(monitor_client, metric_name):
    metrics = await monitor_client.metrics()
    assert len(metrics) > 1
    metric = await monitor_client.single_metric(
        f'ydb.retry_budget.{metric_name}',
    )
    return metric.value


async def test_retry_budget(dynamic_config, service_client, monitor_client):
    approx_token_count = await _get_current_metric(
        monitor_client, 'approx_token_count',
    )
    account_ok = await _get_current_metric(monitor_client, 'account_ok')

    await _check_metrics(
        monitor_client,
        account_ok=account_ok,
        account_fail=0,
        approx_token_count=approx_token_count,
        max_token_count=approx_token_count,
    )

    # update dyn config: increase max-tokens and change token-ratio

    # the default value of token-ratio is 0.1. We set the value to 1 and expect
    # that the 'approx_token_count' will be increased by 1 after each request
    max_token_count = 150
    retry_budget_config = {
        'sampledb': {
            'max-tokens': max_token_count,
            'token-ratio': 1,
            'enabled': True,
        },
    }
    dynamic_config.set_values({'YDB_RETRY_BUDGET': retry_budget_config})
    await service_client.update_server_state()

    await _check_metrics(
        monitor_client,
        account_ok=account_ok,
        account_fail=0,
        approx_token_count=approx_token_count,
        max_token_count=max_token_count,
    )

    await _make_success_request(service_client)

    await _check_metrics(
        monitor_client,
        account_ok=account_ok + 1,
        account_fail=0,
        approx_token_count=approx_token_count + 1,
        max_token_count=max_token_count,
    )

    requests_count = 10
    for _ in range(requests_count):
        await _make_success_request(service_client)

    await _check_metrics(
        monitor_client,
        account_ok=account_ok + requests_count + 1,
        account_fail=0,
        approx_token_count=approx_token_count + requests_count + 1,
        max_token_count=max_token_count,
    )


async def test_retry_budget_success_limit(
        dynamic_config, service_client, monitor_client,
):
    approx_token_count = await _get_current_metric(
        monitor_client, 'approx_token_count',
    )
    account_ok = await _get_current_metric(monitor_client, 'account_ok')
    max_token_count = await _get_current_metric(
        monitor_client, 'max_token_count',
    )

    await _check_metrics(
        monitor_client,
        account_ok=account_ok,
        account_fail=0,
        approx_token_count=approx_token_count,
        max_token_count=max_token_count,
    )

    # set max-tokens as current approx_token_count
    retry_budget_config = {
        'sampledb': {
            'max-tokens': approx_token_count,
            'token-ratio': 1,
            'enabled': True,
        },
    }
    dynamic_config.set_values({'YDB_RETRY_BUDGET': retry_budget_config})
    await service_client.update_server_state()

    # We can't increase the approx_token_count more than the max-tokens,
    # so the approx_token_count won't change
    requests_count = 10
    for _ in range(requests_count):
        await _make_success_request(service_client)

    await _check_metrics(
        monitor_client,
        account_ok=account_ok + requests_count,
        account_fail=0,
        approx_token_count=approx_token_count,
        max_token_count=approx_token_count,
    )
