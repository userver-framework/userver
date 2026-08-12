import asyncio

CHAOS_URL = '/chaos'
CHAOS_TRX_URL = '/chaos/trx'

_MAX_RETRIES = 20
_RETRY_DELAY = 0.05


async def _check_that_restores(client, url=CHAOS_URL):
    for _ in range(_MAX_RETRIES):
        res = await client.delete(url + '?key=foo')
        if res.status == 200:
            return
        await asyncio.sleep(_RETRY_DELAY)

    assert False, 'Service did not recover after connection restore'


async def _check_crud(client, url=CHAOS_URL):
    response = await client.post(url + '?key=foo&value=bar')
    assert response.status_code == 201

    response = await client.get(url + '?key=foo')
    assert response.status_code == 200
    assert response.text == 'bar'

    response = await client.delete(url + '?key=foo')
    assert response.status_code == 200


async def _cleanup(client, *keys):
    for key in keys:
        await client.delete(CHAOS_URL + f'?key={key}')


async def test_odbc_happy_path(service_client):
    await _check_crud(service_client)


async def test_odbc_transaction_happy_path(service_client):
    await _check_crud(service_client, url=CHAOS_TRX_URL)


async def test_odbc_transaction_multi_statement(service_client):
    await _cleanup(service_client, 'multi_key1', 'multi_key2')

    response = await service_client.post(
        CHAOS_TRX_URL + '?key=multi_key1&value=val1',
    )
    assert response.status_code == 201

    response = await service_client.post(
        CHAOS_TRX_URL + '?key=multi_key2&value=val2',
    )
    assert response.status_code == 201

    response = await service_client.get(CHAOS_URL + '?key=multi_key1')
    assert response.status_code == 200
    assert response.text == 'val1'

    response = await service_client.get(CHAOS_URL + '?key=multi_key2')
    assert response.status_code == 200
    assert response.text == 'val2'

    await _cleanup(service_client, 'multi_key1', 'multi_key2')


async def test_odbc_transaction_update_and_read(service_client):
    await _cleanup(service_client, 'upd_key')

    response = await service_client.post(CHAOS_URL + '?key=upd_key&value=old')
    assert response.status_code == 201

    response = await service_client.post(
        CHAOS_TRX_URL + '?key=upd_key&value=new',
    )
    assert response.status_code == 201

    response = await service_client.get(CHAOS_URL + '?key=upd_key')
    assert response.status_code == 200
    assert response.text == 'new'

    await _cleanup(service_client, 'upd_key')


async def test_odbc_transaction_delete_in_trx(service_client):
    await _cleanup(service_client, 'del_key')

    response = await service_client.post(CHAOS_URL + '?key=del_key&value=v')
    assert response.status_code == 201

    response = await service_client.delete(CHAOS_TRX_URL + '?key=del_key')
    assert response.status_code == 200

    response = await service_client.get(CHAOS_URL + '?key=del_key')
    assert response.status_code == 404


async def test_odbc_transaction_read_own_write(service_client):
    await _cleanup(service_client, 'row_key')

    response = await service_client.post(
        CHAOS_TRX_URL + '?key=row_key&value=row_val',
    )
    assert response.status_code == 201

    response = await service_client.get(CHAOS_TRX_URL + '?key=row_key')
    assert response.status_code == 200
    assert response.text == 'row_val'

    await _cleanup(service_client, 'row_key')


async def test_odbc_transaction_sequential(service_client):
    keys = [f'seq_key_{i}' for i in range(5)]
    await _cleanup(service_client, *keys)

    for i, key in enumerate(keys):
        response = await service_client.post(
            CHAOS_TRX_URL + f'?key={key}&value=v{i}',
        )
        assert response.status_code == 201

    for i, key in enumerate(keys):
        response = await service_client.get(CHAOS_URL + f'?key={key}')
        assert response.status_code == 200
        assert response.text == f'v{i}'

    await _cleanup(service_client, *keys)


async def test_odbc_transaction_no_accepts(service_client, gate):
    response = await service_client.get(CHAOS_TRX_URL + '?key=nonexistent')
    assert response.status_code in (200, 404)

    await gate.stop_accepting()

    response = await service_client.get(CHAOS_TRX_URL + '?key=nonexistent')
    assert response.status_code in (200, 404)

    gate.start_accepting()


async def test_odbc_close_connections(service_client, gate):
    response = await service_client.get(CHAOS_URL + '?key=nonexistent')
    assert response.status_code in (200, 404)

    await gate.sockets_close()
    await _check_that_restores(service_client)


async def test_odbc_close_connections_on_write(service_client, gate):
    response = await service_client.post(CHAOS_URL + '?key=foo&value=bar')
    assert response.status_code == 201

    await gate.sockets_close()
    await _check_that_restores(service_client)


async def test_odbc_transaction_close_on_execute(service_client, gate):
    response = await service_client.get(CHAOS_TRX_URL + '?key=nonexistent')
    assert response.status_code in (200, 404)

    await gate.sockets_close()
    await _check_that_restores(service_client, url=CHAOS_TRX_URL)


async def test_odbc_transaction_close_on_write(service_client, gate):
    response = await service_client.post(CHAOS_TRX_URL + '?key=foo&value=bar')
    assert response.status_code == 201

    await gate.sockets_close()
    await _check_that_restores(service_client, url=CHAOS_TRX_URL)


async def test_odbc_no_accepts(service_client, gate):
    response = await service_client.get(CHAOS_URL + '?key=nonexistent')
    assert response.status_code in (200, 404)

    await gate.stop_accepting()

    response = await service_client.get(CHAOS_URL + '?key=nonexistent')
    assert response.status_code in (200, 404)

    gate.start_accepting()
