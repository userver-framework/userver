import uuid


async def test_schema_init_response(service_client):
    response = await service_client.post('/v1/schema/init')

    assert response.status == 200
    body = response.json()

    assert body['status'] == 'ok'
    assert sorted(body['tables']) == ['basic', 'events']


# /// [Functional test]
async def test_kv_crud(service_client):
    response = await service_client.post(
        '/v1/kv',
        json={'key': 'hello', 'bln': True, 'i32': 7, 'dbl': 2.5},
    )
    assert response.status == 200

    response = await service_client.get('/v1/kv', params={'key': 'hello'})
    assert response.status == 200
    body = response.json()
    assert body['key'] == 'hello'
    assert body['bln'] is True
    assert body['i32'] == 7
    assert body['dbl'] == 2.5

    response = await service_client.patch(
        '/v1/kv',
        params={'key': 'hello'},
        json={'i32': 42},
    )
    assert response.status == 200

    response = await service_client.get('/v1/kv', params={'key': 'hello'})
    assert response.json()['i32'] == 42

    response = await service_client.delete('/v1/kv', params={'key': 'hello'})
    assert response.status == 200

    response = await service_client.get('/v1/kv', params={'key': 'hello'})
    assert response.status == 404
    # /// [Functional test]


async def test_kv_missing(service_client):
    response = await service_client.get('/v1/kv', params={'key': 'absent'})
    assert response.status == 404
    assert response.json() == {'error': 'not_found'}


async def test_kv_requires_key(service_client):
    response = await service_client.post('/v1/kv', json={'i32': 1})
    assert response.status == 400


async def test_list_and_count(service_client):
    for i in range(5):
        response = await service_client.post(
            '/v1/kv',
            json={'key': f'k{i}', 'i32': i},
        )
        assert response.status == 200

    response = await service_client.get('/v1/kv/list')
    assert response.status == 200
    body = response.json()
    assert body['count'] == 5
    assert {item['key'] for item in body['items']} == {f'k{i}' for i in range(5)}

    response = await service_client.get('/v1/kv/list', params={'limit': 2})
    assert response.json()['count'] == 2

    response = await service_client.get('/v1/kv/count')
    assert response.json() == {'count': 5}

    response = await service_client.get('/v1/kv/count', params={'key': 'k0'})
    assert response.json() == {'count': 1}


async def test_bulk_insert(service_client):
    payload = [{'key': f'b{i}', 'i64': i * 10} for i in range(4)]
    response = await service_client.post('/v1/kv/bulk', json=payload)
    assert response.status == 200
    assert response.json() == {'inserted': 4}

    response = await service_client.get('/v1/kv/list')
    assert response.json()['count'] == 4


async def test_bulk_rejects_empty(service_client):
    response = await service_client.post('/v1/kv/bulk', json=[])
    assert response.status == 400


async def test_pagination(service_client):
    for i in range(7):
        response = await service_client.post(
            '/v1/kv',
            json={'key': f'p{i}', 'i32': i},
        )
        assert response.status == 200

    seen = set()
    cursor = None
    for _ in range(10):
        params = {'page_size': 3}
        if cursor:
            params['cursor'] = cursor
        response = await service_client.get('/v1/kv/pages', params=params)
        assert response.status == 200
        body = response.json()
        seen.update(item['key'] for item in body['items'])
        if not body['has_more']:
            break
        cursor = body['next_cursor']

    assert seen == {f'p{i}' for i in range(7)}


async def test_create_if_absent(service_client):
    response = await service_client.post(
        '/v1/kv/create_if_absent',
        json={'key': 'lwt', 'i32': 1},
    )
    assert response.status == 200
    assert response.json() == {'applied': True}

    response = await service_client.post(
        '/v1/kv/create_if_absent',
        json={'key': 'lwt', 'i32': 2},
    )
    assert response.status == 409
    body = response.json()
    assert body['applied'] is False
    assert body['existing']['i32'] == 1


async def test_compare_and_set(service_client):
    response = await service_client.post(
        '/v1/kv',
        json={'key': 'cas', 'i32': 1},
    )
    assert response.status == 200

    response = await service_client.post(
        '/v1/kv/cas',
        params={'key': 'cas'},
        json={'expect': {'i32': 1}, 'set': {'i32': 99}},
    )
    assert response.status == 200
    assert response.json() == {'applied': True}

    response = await service_client.get('/v1/kv', params={'key': 'cas'})
    assert response.json()['i32'] == 99

    response = await service_client.post(
        '/v1/kv/cas',
        params={'key': 'cas'},
        json={'expect': {'i32': 1}, 'set': {'i32': 0}},
    )
    assert response.status == 409
    body = response.json()
    assert body['applied'] is False
    assert body['current']['i32'] == 99


async def test_delete_if_exists(service_client):
    response = await service_client.post(
        '/v1/kv',
        json={'key': 'd1'},
    )
    assert response.status == 200

    response = await service_client.delete(
        '/v1/kv/delete_if_exists',
        params={'key': 'd1'},
    )
    assert response.status == 200
    assert response.json() == {'applied': True}

    response = await service_client.delete(
        '/v1/kv/delete_if_exists',
        params={'key': 'd1'},
    )
    assert response.status == 404
    assert response.json() == {'applied': False}


async def test_raw_query(service_client):
    response = await service_client.post(
        '/v1/kv',
        json={'key': 'r1', 'i32': 10},
    )
    assert response.status == 200

    response = await service_client.post(
        '/v1/raw',
        json={'query': 'SELECT key, i32 FROM examples.basic'},
    )
    assert response.status == 200
    body = response.json()
    assert body['count'] == 1
    assert body['rows'][0]['key'] == 'r1'
    assert body['rows'][0]['i32'] == 10


async def test_raw_query_requires_query(service_client):
    response = await service_client.post('/v1/raw', json={})
    assert response.status == 400


async def test_events_crud(service_client):
    response = await service_client.post(
        '/v1/events',
        json={
            'name': 'login',
            'source_ip': '192.168.0.1',
            'tags': ['auth', 'ok'],
            'metadata': {'user': 'alice'},
            'scores': [1, 2, 3],
        },
    )
    assert response.status == 200
    event_id = response.json()['id']

    response = await service_client.get('/v1/events', params={'id': event_id})
    assert response.status == 200
    body = response.json()
    assert body['id'] == event_id
    assert body['name'] == 'login'
    assert body['source_ip'] == '192.168.0.1'
    assert sorted(body['tags']) == ['auth', 'ok']
    assert body['metadata'] == {'user': 'alice'}
    assert body['scores'] == [1, 2, 3]


async def test_events_missing(service_client):
    response = await service_client.get(
        '/v1/events',
        params={'id': '00000000-0000-0000-0000-000000000000'},
    )
    assert response.status == 404


async def test_events_requires_name(service_client):
    response = await service_client.post('/v1/events', json={})
    assert response.status == 400


async def test_events_list(service_client):
    for name in ('a', 'b', 'c'):
        response = await service_client.post('/v1/events', json={'name': name})
        assert response.status == 200

    response = await service_client.get('/v1/events/list')
    assert response.status == 200
    body = response.json()
    assert body['count'] == 3
    assert {item['name'] for item in body['items']} == {'a', 'b', 'c'}


async def test_events_list_page_size(service_client):
    names = {f'e{i}' for i in range(5)}
    for name in names:
        response = await service_client.post('/v1/events', json={'name': name})
        assert response.status == 200

    response = await service_client.get(
        '/v1/events/list', params={'page_size': 2},
    )
    assert response.status == 200
    body = response.json()
    assert body['count'] == 5
    assert {item['name'] for item in body['items']} == names


async def test_kv_insert_with_ttl(service_client):
    response = await service_client.post(
        '/v1/kv',
        json={'key': 'ephemeral', 'i32': 999, 'ttl': 60},
    )
    assert response.status == 200

    response = await service_client.get('/v1/kv', params={'key': 'ephemeral'})
    assert response.status == 200
    assert response.json()['i32'] == 999


async def test_kv_update_with_ttl(service_client):
    response = await service_client.post(
        '/v1/kv',
        json={'key': 'ttl-upd', 'i32': 1},
    )
    assert response.status == 200

    response = await service_client.patch(
        '/v1/kv',
        params={'key': 'ttl-upd'},
        json={'i64': 100, 'ttl': 300},
    )
    assert response.status == 200

    response = await service_client.get('/v1/kv', params={'key': 'ttl-upd'})

    assert response.status == 200
    assert response.json()['i64'] == 100


async def test_event_with_ttl(service_client):
    response = await service_client.post(
        '/v1/events',
        json={'name': 'temp-alert', 'ttl': 3600},
    )
    assert response.status == 200
    event_id = response.json()['id']

    response = await service_client.get('/v1/events', params={'id': event_id})
    assert response.status == 200
    assert response.json()['name'] == 'temp-alert'


async def test_event_with_write_timestamp(service_client):
    response = await service_client.post(
        '/v1/events',
        json={'name': 'backfill', 'write_timestamp': 1700000000000000},
    )
    assert response.status == 200
    event_id = response.json()['id']

    response = await service_client.get('/v1/events', params={'id': event_id})
    assert response.status == 200
    assert response.json()['name'] == 'backfill'


async def test_raw_query_with_params(service_client):
    response = await service_client.post(
        '/v1/kv',
        json={'key': 'param-test', 'i32': 42},
    )
    assert response.status == 200

    response = await service_client.post(
        '/v1/raw',
        json={
            'query': 'SELECT key, i32 FROM examples.basic WHERE key = ?',
            'params': ['param-test'],
        },
    )
    assert response.status == 200
    body = response.json()
    assert body['count'] == 1
    assert body['rows'][0]['key'] == 'param-test'
    assert body['rows'][0]['i32'] == 42


async def test_raw_query_system_table(service_client):
    response = await service_client.post(
        '/v1/raw',
        json={
            'query': 'SELECT cluster_name, release_version FROM system.local',
        },
    )
    assert response.status == 200
    body = response.json()
    assert body['count'] == 1
    row = body['rows'][0]
    assert 'cluster_name' in row
    assert 'release_version' in row


async def test_raw_ddl_void(service_client):
    response = await service_client.post(
        '/v1/raw',
        json={
            'query': (
                'CREATE INDEX IF NOT EXISTS idx_events_name '
                'ON examples.events (name)'
            ),
            'void': True,
        },
    )
    assert response.status == 200
    assert response.json() == {'status': 'ok'}


async def test_raw_aggregation(service_client):
    for name in ('x', 'y'):
        response = await service_client.post('/v1/events', json={'name': name})
        assert response.status == 200

    response = await service_client.post(
        '/v1/raw',
        json={'query': 'SELECT COUNT(*) FROM examples.events'},
    )
    assert response.status == 200
    body = response.json()
    assert body['count'] == 1
    row = body['rows'][0]
    assert row['count'] == 2


async def test_kv_all_basic_types(service_client):
    payload = {
        'key': 'alpha',
        'bln': True,
        'i32': 1,
        'i64': 42,
        'flt': 1.5,
        'dbl': 2.5,
    }
    response = await service_client.post('/v1/kv', json=payload)
    assert response.status == 200

    response = await service_client.get('/v1/kv', params={'key': 'alpha'})
    assert response.status == 200
    body = response.json()

    assert body['key'] == 'alpha'
    assert body['bln'] is True
    assert body['i32'] == 1
    assert body['i64'] == 42
    assert body['flt'] == 1.5
    assert body['dbl'] == 2.5


async def test_pagination_cursor_contract(service_client):
    for i in range(5):
        response = await service_client.post(
            '/v1/kv',
            json={'key': f'c{i}', 'i32': i},
        )
        assert response.status == 200

    response = await service_client.get(
        '/v1/kv/pages', params={'page_size': 2},
    )
    assert response.status == 200
    first = response.json()
    assert first['has_more'] is True
    assert 'next_cursor' in first
    assert isinstance(first['next_cursor'], str)
    assert first['next_cursor']

    cursor = first['next_cursor']
    while True:
        response = await service_client.get(
            '/v1/kv/pages',
            params={'page_size': 2, 'cursor': cursor},
        )
        assert response.status == 200
        body = response.json()
        if not body['has_more']:
            assert 'next_cursor' not in body
            break
        cursor = body['next_cursor']


async def test_event_response_shape(service_client):
    response = await service_client.post('/v1/events', json={'name': 'shape'})
    assert response.status == 200
    body = response.json()

    assert body['status'] == 'ok'

    parsed = uuid.UUID(body['id'])
    assert str(parsed) == body['id']

    assert isinstance(body['created_at'], int)

    assert body['created_at'] > 1577836800000


async def test_raw_full_table_scan(service_client):
    payload = [
        {'key': 'scan-a', 'i32': 1},
        {'key': 'scan-b', 'i32': 2},
        {'key': 'scan-c', 'i32': 3},
    ]
    response = await service_client.post('/v1/kv/bulk', json=payload)
    assert response.status == 200

    response = await service_client.post(
        '/v1/raw',
        json={'query': 'SELECT * FROM examples.basic'},
    )
    assert response.status == 200
    body = response.json()
    assert body['count'] == 3
    assert {row['key'] for row in body['rows']} == {
        'scan-a', 'scan-b', 'scan-c',
    }


async def test_truncate(service_client):
    for i in range(3):
        response = await service_client.post(
            '/v1/kv',
            json={'key': f't{i}', 'i32': i},
        )
        assert response.status == 200

    response = await service_client.get('/v1/kv/count')
    assert response.json() == {'count': 3}

    response = await service_client.post('/v1/kv/truncate')
    assert response.status == 200
    assert response.json() == {'status': 'ok'}

    response = await service_client.get('/v1/kv/count')
    assert response.json() == {'count': 0}
