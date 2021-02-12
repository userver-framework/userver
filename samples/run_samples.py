import contextlib
import http.client
import socket
import subprocess
import time

SECONDS_TO_START = 5.0
SERVICE_PORT = 8080
SERVICE_HOST = 'localhost'


@contextlib.contextmanager
def start_service(service_name, timeout=SECONDS_TO_START):
    service = subprocess.Popen([service_name])

    start_time = time.perf_counter()
    while True:
        if time.perf_counter() - start_time >= timeout:
            raise TimeoutError(
                f'Waited too long for the port {SERVICE_PORT} on host '
                f'{SERVICE_HOST} to start accepting connections.',
            )

        try:
            with socket.create_connection(
                    (SERVICE_HOST, SERVICE_PORT), timeout=timeout,
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


if __name__ == '__main__':
    test_hello()
