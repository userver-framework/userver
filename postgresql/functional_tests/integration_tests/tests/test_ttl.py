async def _make_request(service_client):
    response = await service_client.post('/v1/key-value?key=foo&value=bar')
    assert response.status in (200, 201)
    assert response.content == b'bar'


async def test_connection_max_ttl(service_client, monitor_client, mocked_time):
    await _make_request(service_client)

    async with monitor_client.metrics_diff(
            prefix='postgresql.connections', diff_gauge=True,
    ) as diff:
        # with max-ttl-sec set to 60 we expect the db connection to be reopened
        # on the next request
        mocked_time.sleep(61)
        await _make_request(service_client)

    assert diff.value_at('closed') == 1
    assert diff.value_at('opened') == 1
