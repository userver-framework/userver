import pytest

try:
    from src.proto.grpc.health.v1 import health_pb2_grpc
except ImportError:
    from health.v1 import health_pb2_grpc

pytest_plugins = ['pytest_userver.plugins.grpc']


@pytest.fixture
def grpc_client(grpc_channel):
    return health_pb2_grpc.HealthStub(grpc_channel)
