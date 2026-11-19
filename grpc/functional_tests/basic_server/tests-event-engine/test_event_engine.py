import pathlib

import samples.greeter_pb2 as greeter_protos


def _service_thread_names(config_path: pathlib.Path) -> list[str]:
    for process in pathlib.Path('/proc').glob('[0-9]*'):
        try:
            arguments = (process / 'cmdline').read_bytes().split(b'\0')
            if str(config_path).encode() in arguments:
                return [(task / 'comm').read_text().strip() for task in (process / 'task').iterdir()]
        except OSError:
            continue
    raise RuntimeError(f'No process is running with the config {config_path}')


async def test_say_hello(grpc_client, grpc_service_endpoint):
    assert grpc_service_endpoint.startswith('unix:')
    request = greeter_protos.GreetingRequest(name='Python')
    response = await grpc_client.SayHello(request)
    assert response.greeting == 'Hello, Python!'


async def test_no_nexting_threads(grpc_client, service_config_path_temp):
    await grpc_client.SayHello(greeter_protos.GreetingRequest(name='Python'))
    thread_names = _service_thread_names(service_config_path_temp)
    assert 'grpc-queue' in thread_names
    assert 'nexting_thread' not in thread_names
