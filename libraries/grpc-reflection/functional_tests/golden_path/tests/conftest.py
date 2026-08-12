from grpc_reflection.v1alpha.proto_reflection_descriptor_database import ServerReflectionStub
import pytest

pytest_plugins = ['pytest_userver.plugins.grpc']


@pytest.fixture
def grpc_reflection_client(grpc_channel):
    return ServerReflectionStub(grpc_channel)
