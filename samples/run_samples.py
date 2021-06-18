import contextlib
import http.client
import socket
import subprocess
import time

SECONDS_TO_START = 5.0
SERVICE_PORT = 8080
SERVICE_HOST = 'localhost'


@contextlib.contextmanager
def start_service(service_name, timeout=SECONDS_TO_START, port=SERVICE_PORT):
    service = subprocess.Popen([service_name])

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
    with start_service('./userver-samples-hello_service'):
        conn = http.client.HTTPConnection(SERVICE_HOST, SERVICE_PORT)
        conn.request('GET', '/hello')
        resp = conn.getresponse()
        assert resp.status == 200
        assert resp.read() == b'Hello world!\n'


def test_config():
    port = 8083
    with start_service('./userver-samples-config_service', port=port):
        conn = http.client.HTTPConnection(SERVICE_HOST, port)
        conn.request('POST', '/configs/values', body='{}')
        resp = conn.getresponse()
        assert resp.status == 200
        assert b'"USERVER_LOG_REQUEST_HEADERS":true' in resp.read()


def test_flatbuf():
    port = 8084
    with start_service('./userver-samples-flatbuf_service', port=port):
        conn = http.client.HTTPConnection(SERVICE_HOST, port)
        body = bytearray.fromhex(
            '100000000c00180000000800100004000c000000140000001400000000000000'
            '16000000000000000a00000048656c6c6f20776f72640000',
        )
        conn.request('POST', '/fbs', body=body)
        resp = conn.getresponse()
        assert resp.status == 200


if __name__ == '__main__':
    test_hello()
    test_config()
    test_flatbuf()
