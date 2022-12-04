# /// [Prepare modules]
# pylint: disable=no-member
import sys

import grpc
import pytest
# /// [Prepare modules]

pytest_plugins = ['pytest_userver.plugins', 'pytest_userver.plugins.samples']

# /// [Prepare configs]
USERVER_CONFIG_HOOKS = ['_prepare_service_config']


@pytest.fixture(scope='session')
def _prepare_service_config(grpc_mockserver_endpoint):
    def patch_config(config, config_vars):
        components = config['components_manager']['components']
        components['greeter-client']['endpoint'] = grpc_mockserver_endpoint

    return patch_config
    # /// [Prepare configs]


# /// [Prepare server mock]
@pytest.fixture(scope='session')
def mock_grpc_greeter_session(
        greeter_services, grpc_mockserver, create_grpc_mock,
):
    mock = create_grpc_mock(greeter_services.GreeterServiceServicer)
    greeter_services.add_GreeterServiceServicer_to_server(
        mock.servicer, grpc_mockserver,
    )
    return mock


@pytest.fixture
def mock_grpc_greeter(mock_grpc_greeter_session):
    with mock_grpc_greeter_session.mock() as mock:
        yield mock
        # /// [Prepare server mock]


# /// [grpc load schemes]
def pytest_configure(config):
    sys.path.append(str(config.rootpath / 'grpc_service/proto/samples'))


@pytest.fixture(scope='session')
def greeter_protos():
    return grpc.protos('greeter.proto')


@pytest.fixture(scope='session')
def greeter_services():
    return grpc.services('greeter.proto')
    # /// [grpc load schemes]


# /// [grpc client]
@pytest.fixture
def grpc_client(grpc_channel, greeter_services, service_client):
    return greeter_services.GreeterServiceStub(grpc_channel)
    # /// [grpc client]
