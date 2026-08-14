import pytest

RUNTIME_QUERY_CACHE = 'runtime-query-mongo-cache'
DEFAULT_QUERY_CACHE = 'default-query-mongo-cache'

ALL_DOCUMENTS = {'first': 1, 'second': 2, 'third': 3}


async def _cache_state(service_client):
    response = await service_client.get('/v1/cache-state')
    assert response.status == 200
    return response.json()


@pytest.mark.config(TEST_MONGO_CACHE_KEY_FILTER='second')
async def test_query_is_built_at_runtime(service_client, cached_documents):
    await service_client.invalidate_caches(
        cache_names=[RUNTIME_QUERY_CACHE, DEFAULT_QUERY_CACHE],
    )

    state = await _cache_state(service_client)

    # The query of MakeFindOperation, and not the default one, was sent to
    # mongo: only the document matching the dynamic config is in the cache
    assert state['runtime-query-cache'] == {'second': 2}
    assert state['last-update-type'] == 'full'

    # A query defined in the traits is unaffected: the whole collection is read
    assert state['default-query-cache'] == ALL_DOCUMENTS


@pytest.mark.config(TEST_MONGO_CACHE_KEY_FILTER='second')
async def test_query_follows_the_dynamic_config(
    service_client,
    dynamic_config,
    cached_documents,
):
    await service_client.invalidate_caches(cache_names=[RUNTIME_QUERY_CACHE])
    assert (await _cache_state(service_client))['runtime-query-cache'] == {'second': 2}

    # The query is built anew on every update, so the cache follows the value
    # that is only known at runtime
    dynamic_config.set_values({'TEST_MONGO_CACHE_KEY_FILTER': 'third'})
    await service_client.update_server_state()
    await service_client.invalidate_caches(
        cache_names=[RUNTIME_QUERY_CACHE, DEFAULT_QUERY_CACHE],
    )

    state = await _cache_state(service_client)
    assert state['runtime-query-cache'] == {'third': 3}
    assert state['default-query-cache'] == ALL_DOCUMENTS


@pytest.mark.config(TEST_MONGO_CACHE_KEY_FILTER='nonexistent')
async def test_query_matching_nothing(service_client, cached_documents):
    await service_client.invalidate_caches(cache_names=[RUNTIME_QUERY_CACHE])

    assert (await _cache_state(service_client))['runtime-query-cache'] == {}


@pytest.mark.config(TEST_MONGO_CACHE_KEY_FILTER='second')
async def test_incremental_update(service_client, dynamic_config, cached_documents):
    await service_client.invalidate_caches(cache_names=[RUNTIME_QUERY_CACHE])
    before = await _cache_state(service_client)
    assert before['runtime-query-cache'] == {'second': 2}

    dynamic_config.set_values({'TEST_MONGO_CACHE_KEY_FILTER': 'third'})
    await service_client.update_server_state()

    # The traits specify no kMongoUpdateFieldName, and yet incremental updates
    # work: MakeFindOperation builds their query as well
    await service_client.invalidate_caches(
        clean_update=False,
        cache_names=[RUNTIME_QUERY_CACHE],
    )

    after = await _cache_state(service_client)
    assert after['last-update-type'] == 'incremental'
    assert after['make-find-operation-count'] > before['make-find-operation-count']
    # An incremental update adds to the cache instead of replacing it
    assert after['runtime-query-cache'] == {'second': 2, 'third': 3}
