import pytest
import samples.greeter_pb2_grpc as greeter_services  # noqa: E402, E501

pytest_plugins = ['pytest_userver.plugins.grpc']


@pytest.fixture(name='mock_grpc_greeter_session', scope='session')
def _mock_grpc_greeter_session(grpc_mockserver, create_grpc_mock):
    mock = create_grpc_mock(greeter_services.GreeterServiceServicer)
    greeter_services.add_GreeterServiceServicer_to_server(
        mock.servicer, grpc_mockserver,
    )
    return mock


@pytest.fixture
def mock_grpc_greeter(mock_grpc_greeter_session):
    with mock_grpc_greeter_session.mock() as mock:
        yield mock


@pytest.fixture
def grpc_client(grpc_channel, service_client):
    return greeter_services.GreeterServiceStub(grpc_channel)
