# /// [Prepare modules]
import pytest
import samples.greeter_pb2_grpc as greeter_services  # noqa: E402, E501

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


# /// [Prepare server mock]
@pytest.fixture(scope='session')
def mock_grpc_greeter_session(grpc_mockserver, create_grpc_mock):
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


# /// [grpc client]
@pytest.fixture
def grpc_client(grpc_channel, service_client):
    return greeter_services.GreeterServiceStub(grpc_channel)
    # /// [grpc client]
