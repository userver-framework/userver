async def test_inc_update(service_client, testpoint):
    @testpoint('reset-cache-dynamic-config-client-updater')
    def tp_reset(data):
        pass

    await service_client.update_server_state()

    assert tp_reset.times_called <= 1
    if tp_reset.times_called == 1:
        assert tp_reset.next_call() == {'data': {'update_type': 'incremental'}}

    await service_client.invalidate_caches(
        cache_names=['dynamic-config-client-updater'],
    )

    assert tp_reset.times_called == 1
    assert tp_reset.next_call() == {'data': {'update_type': 'full'}}
