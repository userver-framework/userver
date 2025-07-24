import pathlib

import grpc
import pytest

import samples.greeter_pb2_grpc as greeter_services

pytest_plugins = ['pytest_userver.plugins.grpc']

# /// [Prepare service config]
USERVER_CONFIG_HOOKS = ['prepare_service_config']

TESTDIR = pathlib.Path(__file__).parent


@pytest.fixture(scope='session')
def prepare_service_config():
    def _do_patch(config_yaml, config_vars):
        components = config_yaml['components_manager']['components']
        components['grpc-server']['tls'] = {
            'key': str(TESTDIR / 'private_key.key'),
            'cert': str(TESTDIR / 'cert.crt'),
        }

    return _do_patch


# /// [Prepare service config]


@pytest.fixture
def grpc_client(grpc_channel):
    return greeter_services.GreeterServiceStub(grpc_channel)


@pytest.fixture(scope='session')
async def grpc_session_channel(grpc_service_endpoint):
    with open(TESTDIR / 'cert.crt', 'rb') as fi:
        root_ca = fi.read()
    credentials = grpc.ssl_channel_credentials(root_ca)
    async with grpc.aio.secure_channel(
        grpc_service_endpoint,
        credentials,
    ) as channel:
        yield channel
