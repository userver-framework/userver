import datetime

import pytest

SMALL_CHUNK_CACHE = 'key-value-small-chunk-cache'
CACHE_NAMES = ['key-value-pg-cache', 'key-value-no-portal-cache', SMALL_CHUNK_CACHE]


def _insert_row(pgsql, mocked_time, key: str, value: str | None) -> None:
    updated = mocked_time.now().replace(tzinfo=datetime.timezone.utc)
    pgsql['key_value'].cursor().execute(
        'INSERT INTO key_value_table (key, value, updated) VALUES (%s, %s, %s)',
        (key, value, updated),
    )


def _insert(pgsql, mocked_time, key: str) -> None:
    _insert_row(pgsql, mocked_time, key, f'value_{key}')


async def _update_incrementally(service_client, mocked_time, cache_name) -> None:
    mocked_time.sleep(10)
    await service_client.invalidate_caches(
        clean_update=False,
        cache_names=[cache_name],
    )


async def _incremental_update_span(service_client, mocked_time, cache_name) -> dict:
    async with service_client.capture_logs() as capture:
        await _update_incrementally(service_client, mocked_time, cache_name)
    records = capture.select(
        stopwatch_name=f'update-task/{cache_name}',
        update_type='incremental',
    )
    assert len(records) == 1, records
    return records[0]


async def _incremental_documents_read(monitor_client, cache_name) -> int:
    metric = await monitor_client.single_metric(
        'cache.incremental.documents.read_count',
        labels={'cache_name': cache_name},
    )
    return metric.value


async def _cache_keys(service_client, cache_name) -> list[str]:
    response = await service_client.get('/cache/keys', params={'cache': cache_name})
    assert response.status == 200
    return response.json()


@pytest.mark.parametrize('cache_name', CACHE_NAMES)
@pytest.mark.now('2026-01-01T00:00:00+0000')
async def test_incremental_update_copies_data_only_on_changes(
    service_client,
    pgsql,
    mocked_time,
    cache_name,
):
    await service_client.invalidate_caches(cache_names=[cache_name])

    mocked_time.sleep(10)
    _insert(pgsql, mocked_time, 'key_1')
    span = await _incremental_update_span(service_client, mocked_time, cache_name)
    assert 'copy_data_time' in span
    assert span['documents_count'] == '1'

    span = await _incremental_update_span(service_client, mocked_time, cache_name)
    assert 'copy_data_time' not in span
    assert span['documents_count'] == '1'

    mocked_time.sleep(10)
    _insert(pgsql, mocked_time, 'key_2')
    span = await _incremental_update_span(service_client, mocked_time, cache_name)
    assert span['documents_count'] == '2'
    assert await _cache_keys(service_client, cache_name) == ['key_1', 'key_2']


@pytest.mark.parametrize('cache_name', CACHE_NAMES)
@pytest.mark.now('2026-01-01T00:00:00+0000')
async def test_full_update_clears_deleted_data(
    service_client,
    pgsql,
    mocked_time,
    cache_name,
):
    _insert(pgsql, mocked_time, 'key_1')
    _insert(pgsql, mocked_time, 'key_2')
    await service_client.invalidate_caches(cache_names=[cache_name])
    assert await _cache_keys(service_client, cache_name) == ['key_1', 'key_2']

    pgsql['key_value'].cursor().execute('DELETE FROM key_value_table')
    await _update_incrementally(service_client, mocked_time, cache_name)
    assert await _cache_keys(service_client, cache_name) == ['key_1', 'key_2']

    await service_client.invalidate_caches(cache_names=[cache_name])
    assert await _cache_keys(service_client, cache_name) == []


@pytest.mark.parametrize('cache_name', CACHE_NAMES)
@pytest.mark.now('2026-01-01T00:00:00+0000')
async def test_invalid_row_does_not_break_update(
    service_client,
    pgsql,
    mocked_time,
    cache_name,
):
    _insert(pgsql, mocked_time, 'key_1')
    _insert_row(pgsql, mocked_time, 'key_2', None)
    await service_client.invalidate_caches(cache_names=[cache_name])
    assert await _cache_keys(service_client, cache_name) == ['key_1']

    mocked_time.sleep(10)
    _insert_row(pgsql, mocked_time, 'key_3', None)
    _insert(pgsql, mocked_time, 'key_4')
    await _update_incrementally(service_client, mocked_time, cache_name)
    assert await _cache_keys(service_client, cache_name) == ['key_1', 'key_4']


@pytest.mark.parametrize(
    'delta_size, expected_portals',
    [
        pytest.param(1, 1, id='less_than_chunk'),
        pytest.param(2, 2, id='equal_to_chunk'),
        pytest.param(5, 2, id='larger_than_chunk'),
    ],
)
@pytest.mark.now('2026-01-01T00:00:00+0000')
async def test_incremental_update_with_delta_around_chunk_size(
    service_client,
    monitor_client,
    pgsql,
    mocked_time,
    delta_size,
    expected_portals,
):
    _insert(pgsql, mocked_time, 'key_0')
    mocked_time.sleep(10)
    await service_client.invalidate_caches(cache_names=[SMALL_CHUNK_CACHE])

    mocked_time.sleep(10)
    keys = [f'key_{i}' for i in range(1, delta_size + 1)]
    for key in keys:
        _insert(pgsql, mocked_time, key)
    documents_read_before = await _incremental_documents_read(monitor_client, SMALL_CHUNK_CACHE)
    async with service_client.capture_logs() as capture:
        await _update_incrementally(service_client, mocked_time, SMALL_CHUNK_CACHE)
    documents_read = await _incremental_documents_read(monitor_client, SMALL_CHUNK_CACHE) - documents_read_before
    assert documents_read == delta_size

    [span] = capture.select(
        stopwatch_name=f'update-task/{SMALL_CHUNK_CACHE}',
        update_type='incremental',
    )
    assert 'copy_data_time' in span
    assert span['documents_count'] == str(delta_size + 1)
    portals = [
        record
        for record in capture.select(trace_id=span['trace_id'])
        if record.get('stopwatch_name', '').startswith('pg_bind')
    ]
    assert len(portals) == expected_portals
    assert await _cache_keys(service_client, SMALL_CHUNK_CACHE) == ['key_0', *keys]
