import pytest
import pytest_userver.config

import samples.greeter_pb2_grpc as greeter_services

pytest_plugins = ['pytest_userver.plugins.grpc']


@pytest.fixture(scope='session')
@pytest_userver.config.patch
def event_engine_config():
    def patch_config(config_yaml, config_vars):
        grpc_server = config_yaml['components_manager']['components']['grpc-server']
        grpc_server['use-callback-api'] = True
        grpc_server['event-engine'] = {'enabled': True}

    return patch_config


@pytest.fixture
def grpc_client(grpc_channel):
    return greeter_services.GreeterServiceStub(grpc_channel)
