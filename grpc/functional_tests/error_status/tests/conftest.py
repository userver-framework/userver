import functional_tests.error_status_pb2_grpc as error_status_services
import pytest
from raw_http2_client import RawGrpcClient

pytest_plugins = ['pytest_userver.plugins.grpc']


@pytest.fixture
def greeter_mock(grpc_mockserver):
    return grpc_mockserver.mock_factory(error_status_services.ErrorStatusServiceServicer)


@pytest.fixture
def grpc_client(grpc_channel):
    return error_status_services.ErrorStatusServiceStub(grpc_channel)


@pytest.fixture
def raw_grpc_client(
    service_client,  # For daemon setup and userver_client_cleanup
    grpc_service_endpoint,
) -> RawGrpcClient:
    host, port = grpc_service_endpoint.rsplit(':', 1)
    return RawGrpcClient(host, int(port))
