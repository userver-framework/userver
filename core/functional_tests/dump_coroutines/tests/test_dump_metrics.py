SYMBOL = 'server::net::Connection::ListenForRequests()'


async def test_dump_coroutines(monitor_client):
    response = await monitor_client.get('service/dump-coroutines')
    assert response.status == 200
    resp = response.json()

    listeners = list(filter(lambda entry: SYMBOL in entry['stacktrace'], resp))
    assert len(listeners) == 2
