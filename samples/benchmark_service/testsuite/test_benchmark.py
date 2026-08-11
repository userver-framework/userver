import gzip


def _seed_items(pgsql, rows):
    cursor = pgsql['admin'].cursor()
    cursor.execute('TRUNCATE items')
    for row in rows:
        cursor.execute(
            'INSERT INTO items '
            '(id, name, category, price, quantity, active, tags, rating_score, rating_count) '
            'VALUES (%s, %s, %s, %s, %s, %s, %s::jsonb, %s, %s)',
            row,
        )


SAMPLE_ITEMS = [
    (1, 'Alpha', 'electronics', 20, 2, True, '["sale"]', 48, 53),
    (2, 'Beta', 'tools', 35, 5, True, '["fast","new"]', 15, 259),
    (3, 'Gamma', 'books', 80, 1, False, '["eco"]', 1, 389),
    (4, 'Delta', 'home', 120, 3, True, '["premium"]', 40, 10),
    (5, 'Epsilon', 'sports', 15, 8, True, '["sale","popular"]', 23, 310),
]


async def test_pipeline(service_client):
    response = await service_client.get('/pipeline')
    assert response.status == 200
    assert response.headers['Content-Type'].startswith('text/plain')
    assert response.text == 'ok'


async def test_baseline11_get(service_client):
    response = await service_client.get('/baseline11', params={'a': '2', 'b': '3'})
    assert response.status == 200
    assert response.headers['Content-Type'].startswith('text/plain')
    assert response.text == '5'


async def test_baseline11_post_with_body(service_client):
    response = await service_client.post(
        '/baseline11',
        params={'a': '2', 'b': '3'},
        data='4',
    )
    assert response.status == 200
    assert response.text == '9'


async def test_baseline11_post_without_body(service_client):
    response = await service_client.post(
        '/baseline11',
        params={'a': '7', 'b': '8'},
        data='',
    )
    assert response.status == 200
    assert response.text == '15'


async def test_baseline2(service_client):
    response = await service_client.get('/baseline2', params={'a': '10', 'b': '20'})
    assert response.status == 200
    assert response.headers['Content-Type'].startswith('text/plain')
    assert response.text == '30'


async def test_json(service_client):
    response = await service_client.get('/json/2', params={'m': '2'})
    assert response.status == 200
    assert response.headers['Content-Type'].startswith('application/json')
    body = response.json()
    assert body['count'] == 2
    assert len(body['items']) == 2
    item = body['items'][0]
    assert item['total'] == item['price'] * item['quantity'] * 2


async def test_json_default_multiplier(service_client):
    response = await service_client.get(
        '/json/1',
        headers={'Accept-Encoding': 'identity'},
    )
    assert response.status == 200
    body = response.json()
    assert body['count'] == 1
    item = body['items'][0]
    assert item['total'] == item['price'] * item['quantity'] * 1.0


async def test_json_count_clamped_to_dataset_size(service_client):
    response = await service_client.get(
        '/json/9999',
        headers={'Accept-Encoding': 'identity'},
    )
    assert response.status == 200
    body = response.json()
    assert body['count'] == 50
    assert len(body['items']) == 50


async def test_json_negative_count(service_client):
    response = await service_client.get(
        '/json/-5',
        headers={'Accept-Encoding': 'identity'},
    )
    assert response.status == 200
    body = response.json()
    assert body['count'] == 0
    assert body['items'] == []


async def test_json_zero_count(service_client):
    response = await service_client.get(
        '/json/0',
        headers={'Accept-Encoding': 'identity'},
    )
    assert response.status == 200
    body = response.json()
    assert body['count'] == 0
    assert body['items'] == []


async def test_json_gzip(service_client):
    uncompressed = await service_client.get(
        '/json/1',
        headers={'Accept-Encoding': 'identity'},
    )
    assert uncompressed.status == 200
    uncompressed_len = int(uncompressed.headers['Content-Length'])
    assert uncompressed_len == len(uncompressed.content)

    response = await service_client.get(
        '/json/1',
        headers={'Accept-Encoding': 'gzip'},
        auto_decompress=False,
    )
    assert response.status == 200
    assert response.headers.get('Content-Encoding') == 'gzip'
    content_length = int(response.headers['Content-Length'])
    assert content_length == len(response.content)
    assert content_length < uncompressed_len
    assert response.content[:2] == b'\x1f\x8b'
    payload = gzip.decompress(response.content)
    assert b'"count"' in payload
    assert b'"items"' in payload


async def test_json_without_gzip(service_client):
    response = await service_client.get(
        '/json/1',
        headers={'Accept-Encoding': 'identity'},
    )
    assert response.status == 200
    assert response.headers.get('Content-Encoding') != 'gzip'
    body = response.json()
    assert body['count'] == 1


async def test_upload(service_client):
    payload = b'hello-benchmark'
    response = await service_client.post('/upload', data=payload)
    assert response.status == 200
    assert response.headers['Content-Type'].startswith('text/plain')
    assert response.text == str(len(payload))


async def test_upload_empty(service_client):
    response = await service_client.post('/upload', data=b'')
    assert response.status == 200
    assert response.text == '0'


async def test_static(service_client):
    response = await service_client.get('/static/manifest.json')
    assert response.status == 200
    assert b'{' in response.content


async def test_static_other_files(service_client):
    for path in ('/static/reset.css', '/static/logo.svg', '/static/footer.html'):
        response = await service_client.get(path)
        assert response.status == 200
        assert len(response.content) > 0


async def test_async_db(service_client, pgsql):
    _seed_items(pgsql, SAMPLE_ITEMS)
    response = await service_client.get(
        '/async-db',
        params={'min': '10', 'max': '50', 'limit': '5'},
    )
    assert response.status == 200
    assert response.headers['Content-Type'].startswith('application/json')
    body = response.json()
    assert body['count'] == 3  # prices 20, 35, 15
    assert len(body['items']) == 3
    for item in body['items']:
        assert 10 <= item['price'] <= 50
        assert 'rating' in item
        assert 'score' in item['rating']
        assert 'count' in item['rating']
        assert 'tags' in item
        assert isinstance(item['active'], bool)


async def test_async_db_defaults(service_client, pgsql):
    _seed_items(pgsql, SAMPLE_ITEMS)
    response = await service_client.get('/async-db')
    assert response.status == 200
    body = response.json()
    # Defaults: min=10, max=50, limit=50
    assert body['count'] == 3
    for item in body['items']:
        assert 10 <= item['price'] <= 50


async def test_async_db_partial_args(service_client, pgsql):
    _seed_items(pgsql, SAMPLE_ITEMS)
    # min=100 with default max=50 yields an empty price range.
    response = await service_client.get(
        '/async-db',
        params={'min': '100', 'limit': '3'},
    )
    assert response.status == 200
    body = response.json()
    assert body['count'] == 0
    assert body['items'] == []


async def test_async_db_wide_range(service_client, pgsql):
    _seed_items(pgsql, SAMPLE_ITEMS)
    response = await service_client.get(
        '/async-db',
        params={'min': '1', 'max': '500', 'limit': '10'},
    )
    assert response.status == 200
    body = response.json()
    assert body['count'] == 5
    assert len(body['items']) == 5
    item = body['items'][0]
    assert set(item) >= {
        'id',
        'name',
        'category',
        'price',
        'quantity',
        'active',
        'tags',
        'rating',
    }


async def test_async_db_limit(service_client, pgsql):
    _seed_items(pgsql, SAMPLE_ITEMS)
    response = await service_client.get(
        '/async-db',
        params={'min': '1', 'max': '500', 'limit': '2'},
    )
    assert response.status == 200
    body = response.json()
    assert body['count'] == 2
    assert len(body['items']) == 2
