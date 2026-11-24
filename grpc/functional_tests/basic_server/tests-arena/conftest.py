import copy

import pytest

import samples.greeter_pb2_grpc as greeter_services

pytest_plugins = ['pytest_userver.plugins.grpc']


@pytest.fixture(
    scope='session', params=[(None, None, False), (True, None, True), (False, True, True), (True, False, False)]
)
def arena_config(request):
    return request.param


@pytest.fixture(scope='session')
def service_config_yaml(service_config_yaml, arena_config):
    config = copy.deepcopy(service_config_yaml)
    components = config['components_manager']['components']
    default, override, _ = arena_config
    if default is not None:
        components['grpc-server'].setdefault('service-defaults', {})['use-protobuf-arena'] = default
    if override is not None:
        components['greeter-service']['use-protobuf-arena'] = override
    return config


@pytest.fixture
def grpc_client(grpc_channel):
    return greeter_services.GreeterServiceStub(grpc_channel)
