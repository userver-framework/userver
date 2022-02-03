import http.client
import os
import urllib.parse

SERVICE_HOST = 'localhost'

CONFIGS_PORT = 8083


def test_hello(start_service):
    port = 8080
    with start_service('hello_service', port=port):
        conn = http.client.HTTPConnection(SERVICE_HOST, port=port)
        conn.request('GET', '/hello')
        with conn.getresponse() as resp:
            assert resp.status == 200
            assert resp.read() == b'Hello world!\n'


def test_config(start_service):
    with start_service('config_service', port=CONFIGS_PORT):
        conn = http.client.HTTPConnection(SERVICE_HOST, CONFIGS_PORT)
        conn.request('POST', '/configs/values', body='{}')
        with conn.getresponse() as resp:
            assert resp.status == 200
            assert b'"USERVER_LOG_REQUEST_HEADERS":true' in resp.read()


def test_flatbuf(start_service):
    port = 8084
    with start_service('flatbuf_service', port=port):
        conn = http.client.HTTPConnection(SERVICE_HOST, port)
        body = bytearray.fromhex(
            '100000000c00180000000800100004000c000000140000001400000000000000'
            '16000000000000000a00000048656c6c6f20776f72640000',
        )
        conn.request('POST', '/fbs', body=body)
        with conn.getresponse() as resp:
            assert resp.status == 200


def test_production_service(start_service):
    with start_service('config_service', port=CONFIGS_PORT):
        port = 8086
        with start_service('production_service', port=port):
            conn = http.client.HTTPConnection(SERVICE_HOST, port)
            conn.request('GET', '/service/log-level/')
            resp = conn.getresponse()
            assert resp.status == 200


def test_postgres(start_service):
    port = 8087
    with start_service('postgres_service', port=port):
        conn = http.client.HTTPConnection(SERVICE_HOST, port)

        conn.request('DELETE', '/v1/key-value?key=hello')
        with conn.getresponse() as resp:
            assert resp.status == 200

        conn.request('POST', '/v1/key-value?key=hello&value=world')
        with conn.getresponse() as resp:
            assert resp.status == 201
            assert resp.read() == b'world'

        conn.request('GET', '/v1/key-value?key=hello')
        with conn.getresponse() as resp:
            assert resp.status == 200
            assert resp.read() == b'world'

        conn.request('DELETE', '/v1/key-value?key=hello')
        with conn.getresponse() as resp:
            assert resp.status == 200

        conn.request('POST', '/v1/key-value?key=hello&value=there')
        with conn.getresponse() as resp:
            assert resp.status == 201
            assert resp.read() == b'there'

        conn.request('GET', '/v1/key-value?key=hello')
        with conn.getresponse() as resp:
            assert resp.status == 200
            assert resp.read() == b'there'

        conn.request('POST', '/v1/key-value?key=hello&value=again')
        with conn.getresponse() as resp:
            assert resp.status == 409  # Conflict
            assert resp.read() == b'there'

        conn.request('GET', '/v1/key-value?key=missing')
        with conn.getresponse() as resp:
            assert resp.status == 404  # Not Found


def test_redis(start_service):
    port = 8088
    os.environ[
        'SECDIST_CONFIG'
    ] = """{
        "redis_settings": {
            "taxi-tmp": {
                "password": "",
                "sentinels": [
                    {"host": "localhost", "port": 26379}
                ],
                "shards": [
                    {"name": "test_master0"}
                ]
            }
        }
    }"""
    with start_service('redis_service', port=port):
        conn = http.client.HTTPConnection(SERVICE_HOST, port)

        conn.request('DELETE', '/v1/key-value?key=hello')
        with conn.getresponse() as resp:
            assert resp.status == 200

        conn.request('POST', '/v1/key-value?key=hello&value=world')
        with conn.getresponse() as resp:
            assert resp.status == 201
            assert resp.read() == b'world'

        conn.request('GET', '/v1/key-value?key=hello')
        with conn.getresponse() as resp:
            assert resp.status == 200
            assert resp.read() == b'world'

        conn.request('POST', '/v1/key-value?key=hello&value=there')
        with conn.getresponse() as resp:
            assert resp.status == 409  # Conflict

        conn.request('GET', '/v1/key-value?key=hello')
        with conn.getresponse() as resp:
            assert resp.status == 200
            assert resp.read() == b'world'  # Still the same

        conn.request('DELETE', '/v1/key-value?key=hello')
        with conn.getresponse() as resp:
            assert resp.status == 200
            assert resp.read() == b'1'

        conn.request('GET', '/v1/key-value?key=hello')
        with conn.getresponse() as resp:
            assert resp.status == 404  # Not Found


def test_http_cache_and_mongo(start_service):
    cache_port = 8089
    mongo_port = 8090
    with start_service('mongo_service', port=mongo_port):
        conn_mongo = http.client.HTTPConnection(SERVICE_HOST, mongo_port)

        data = {
            ('hello', 'ru', urllib.parse.quote('Привет')),
            ('hello', 'en', 'hello'),
            ('wellcome', 'ru', urllib.parse.quote('Добро пожаловать')),
            ('wellcome', 'en', 'Wellcome'),
        }

        for key, lang, value in data:
            url = '/v1/translations?key={}&lang={}&value={}'.format(
                key, lang, value,
            )
            conn_mongo.request(method='PATCH', url=url)
            with conn_mongo.getresponse() as resp:
                assert resp.status == 201, 'Failed: ' + url

        with start_service('http_caching', port=cache_port):
            conn_cache = http.client.HTTPConnection(SERVICE_HOST, cache_port)

            username = urllib.parse.quote('дорогой разработчик')
            conn_cache.request('POST', '/samples/greet?username=' + username)
            with conn_cache.getresponse() as resp:
                assert resp.status == 200
                data = resp.read().decode('utf-8')
                assert (
                    data == 'Привет, дорогой разработчик! Добро пожаловать'
                ), ('Data is: ' + data)

            conn_mongo.request(
                'PATCH',
                '/v1/translations?key=hello&lang=ru&'
                'value=' + urllib.parse.quote('Прив'),
            )
            with conn_mongo.getresponse() as resp:
                assert resp.status == 201

            conn_mongo = http.client.HTTPConnection(SERVICE_HOST, mongo_port)
            conn_mongo.request(
                'PATCH',
                '/v1/translations?key=hello&lang=ru&'
                'value=' + urllib.parse.quote('Приветище'),
            )
            with conn_mongo.getresponse() as resp:
                assert resp.status == 201

            update_cache = """
                {"invalidate_caches": {
                    "update_type": "incremental",
                    "names": ["cache-http-translations"]
                }}
            """
            conn_cache.request('POST', '/tests/control', body=update_cache)
            with conn_cache.getresponse() as resp:
                assert resp.status == 200, resp.read().decode('utf-8')

            conn_cache.request('POST', '/samples/greet?username=' + username)
            with conn_cache.getresponse() as resp:
                assert resp.status == 200
                data = resp.read().decode('utf-8')
                assert (
                    data == 'Приветище, дорогой разработчик! Добро пожаловать'
                ), ('Data is: ' + data)


def test_grpc(start_service):
    port = 8092
    with start_service('grpc_service', port=port):
        conn = http.client.HTTPConnection(SERVICE_HOST, port=port)

        headers = {'Content-type': 'text/plain'}
        conn.request('POST', '/hello', body='tests', headers=headers)
        with conn.getresponse() as resp:
            assert resp.status == 200
            assert resp.read() == b'Hello, tests!'
