import testsuite


def assert_alerts(alerts, exp_seconds):
    assert alerts == [
        {
            'id': 'cache_update_error',
            'message': (
                'cache \'alert-cache\' hasn\'t been updated'
                + f' for {exp_seconds} times'
            ),
        },
    ]


async def invalidate_caches(service_client, update_type):
    if update_type == 'full':
        await service_client.invalidate_caches(cache_names=['alert-cache'])
    elif update_type == 'incremental':
        try:
            await service_client.invalidate_caches(
                clean_update=False, cache_names=['alert-cache'],
            )
        except testsuite.utils.http.HttpResponseError:
            pass


async def wait_to_fire(
        service_client, monitor_client, failure_count, update_interval,
):
    for _ in range(failure_count):
        await invalidate_caches(service_client, 'incremental')


async def test_cache_update(
        service_client, monitor_client, dynamic_config, service_config_yaml,
):
    await service_client.update_server_state()
    alert_cache = service_config_yaml['components_manager']['components'][
        'alert-cache'
    ]
    static_alert = alert_cache['alert-on-failing-to-update-times']
    update_interval = alert_cache['update-interval']
    assert update_interval[-1] == 's'
    update_interval = int(update_interval[:-1])

    await wait_to_fire(
        service_client, monitor_client, static_alert, update_interval,
    )

    assert_alerts(await monitor_client.fired_alerts(), static_alert)

    # successful update
    await invalidate_caches(service_client, 'full')
    assert await monitor_client.fired_alerts() == []

    # set a new value in dynamic_config
    dynamic_alert = static_alert - 1
    assert dynamic_alert != 0
    alert_cache = {
        'alert-cache': {
            'alert-on-failing-to-update-times': dynamic_alert,
            'update-interval-ms': update_interval * 1000,
            'update-jitter-ms': 1000,
            'full-update-interval-ms': 10000,
        },
    }
    dynamic_config.set_values({'USERVER_CACHES': alert_cache})
    await service_client.update_server_state()
    assert dynamic_config.get('USERVER_CACHES') == alert_cache

    await wait_to_fire(
        service_client, monitor_client, dynamic_alert, update_interval,
    )

    assert_alerts(await monitor_client.fired_alerts(), dynamic_alert)
