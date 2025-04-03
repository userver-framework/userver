# /// [Prepare modules]
import pytest

import samples.greeter_pb2_grpc as greeter_services

pytest_plugins = ['pytest_userver.plugins.grpc']
# /// [Prepare modules]

# /// [Prepare configs]
USERVER_CONFIG_HOOKS = ['prepare_service_config']


@pytest.fixture(scope='session')
def prepare_service_config(grpc_mockserver_endpoint):
    def patch_config(config, config_vars):
        components = config['components_manager']['components']
        components['greeter-client']['endpoint'] = grpc_mockserver_endpoint

    return patch_config
    # /// [Prepare configs]


# /// [grpc client]
@pytest.fixture
def grpc_client(grpc_channel):
    return greeter_services.GreeterServiceStub(grpc_channel)
    # /// [grpc client]
