import pathlib

import grpc
import pytest
import pytest_userver.config

import samples.greeter_pb2_grpc as greeter_services

pytest_plugins = ['pytest_userver.plugins.grpc']

TLS_DIR = pathlib.Path(__file__).parent.parent / 'tests-tls'


@pytest.fixture(scope='session')
@pytest_userver.config.patch
def event_engine_tls_config(get_free_port):
    def patch_config(config_yaml, config_vars):
        grpc_server = config_yaml['components_manager']['components']['grpc-server']
        grpc_server['use-callback-api'] = True
        grpc_server['event-engine'] = {'enabled': True, 'task-processor': 'main-task-processor'}
        grpc_server['tls'] = {
            'key': str(TLS_DIR / 'private_key.key'),
            'cert': str(TLS_DIR / 'cert.crt'),
        }
        grpc_server.pop('unix-socket-path', None)
        grpc_server['port'] = get_free_port()

    return patch_config


@pytest.fixture
def grpc_client(grpc_channel):
    return greeter_services.GreeterServiceStub(grpc_channel)


@pytest.fixture(scope='session')
async def grpc_session_channel(grpc_service_endpoint):
    credentials = grpc.ssl_channel_credentials((TLS_DIR / 'cert.crt').read_bytes())
    async with grpc.aio.secure_channel(grpc_service_endpoint, credentials) as channel:
        yield channel
