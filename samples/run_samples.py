import contextlib
import glob
import http.client
import os
import shutil
import socket
import subprocess
import sys
import tempfile
import time

SECONDS_TO_START = 5.0
SERVICE_HOST = 'localhost'

CONFIGS_PORT = 8083

THIS_DIR = os.path.dirname(os.path.realpath(__file__))


def copy_production_service_configs(new_dir: str) -> None:
    config_files_list = glob.glob(THIS_DIR + '/production_service/*')
    for config_file_path in config_files_list:
        with open(config_file_path) as conf_file:
            conf = conf_file.read()

        conf = conf.replace('/etc/production_service', new_dir)
        conf = conf.replace('/var/cache/production_service', new_dir)
        conf = conf.replace('/var/log/production_service', new_dir)
        conf = conf.replace('/var/run/production_service', new_dir)

        new_path = os.path.join(new_dir, os.path.basename(config_file_path))
        print(new_path)
        with open(new_path, 'w') as new_conf_file:
            new_conf_file.write(conf)


@contextlib.contextmanager
def start_service(service_start_args, port, timeout=SECONDS_TO_START):
    service = subprocess.Popen(service_start_args)

    start_time = time.perf_counter()
    while True:
        if time.perf_counter() - start_time >= timeout:
            service.terminate()
            raise TimeoutError(
                (
                    'Waited too long for the port {port} on host '
                    '{SERVICE_HOST} to start accepting connections.'
                ).format(port=port, SERVICE_HOST=SERVICE_HOST),
            )

        try:
            with socket.create_connection(
                    (SERVICE_HOST, port), timeout=timeout,
            ):
                break
        except OSError:
            time.sleep(0.1)

    yield service

    service.terminate()


def test_hello():
    port = 8080
    with start_service(['./userver-samples-hello_service'], port=port):
        conn = http.client.HTTPConnection(SERVICE_HOST, port=port)
        conn.request('GET', '/hello')
        with conn.getresponse() as resp:
            assert resp.status == 200
            assert resp.read() == b'Hello world!\n'


def test_config():
    with start_service(['./userver-samples-config_service'], CONFIGS_PORT):
        conn = http.client.HTTPConnection(SERVICE_HOST, CONFIGS_PORT)
        conn.request('POST', '/configs/values', body='{}')
        with conn.getresponse() as resp:
            assert resp.status == 200
            assert b'"USERVER_LOG_REQUEST_HEADERS":true' in resp.read()


def test_flatbuf():
    port = 8084
    with start_service(['./userver-samples-flatbuf_service'], port=port):
        conn = http.client.HTTPConnection(SERVICE_HOST, port)
        body = bytearray.fromhex(
            '100000000c00180000000800100004000c000000140000001400000000000000'
            '16000000000000000a00000048656c6c6f20776f72640000',
        )
        conn.request('POST', '/fbs', body=body)
        with conn.getresponse() as resp:
            assert resp.status == 200


def test_production_service():
    with start_service(['./userver-samples-config_service'], CONFIGS_PORT):
        with tempfile.TemporaryDirectory() as tmpdirname:
            copy_production_service_configs(tmpdirname)
            port = 8085
            with start_service(
                    [
                        './userver-samples-production_service',
                        '--config',
                        os.path.join(tmpdirname, 'static_config.yaml'),
                    ],
                    port=port,
            ):
                conn = http.client.HTTPConnection(SERVICE_HOST, port)
                conn.request('GET', '/service/log-level/')
                resp = conn.getresponse()
                assert resp.status == 200


def test_postgres():
    port = 8086
    with start_service(['./userver-samples-postgres_service'], port=port):
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


def test_redis():
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
    with start_service(['./userver-samples-redis_service'], port=port):
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


if __name__ == '__main__':
    if '--only-prepare-production-configs' in sys.argv:
        PRODUCTION_SERVICE_CFG_PATH = '/tmp/userver/production_service'

        shutil.rmtree(PRODUCTION_SERVICE_CFG_PATH, ignore_errors=True)
        os.makedirs(PRODUCTION_SERVICE_CFG_PATH)
        copy_production_service_configs(PRODUCTION_SERVICE_CFG_PATH)
        sys.exit(0)

    test_hello()
    test_config()
    test_flatbuf()
    test_production_service()
    test_postgres()
    test_redis()
