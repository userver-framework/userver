import logging

from grpc_reflection.v1alpha.proto_reflection_descriptor_database import ServerReflectionRequest

logger = logging.getLogger(__name__)


async def test_happy_path(grpc_reflection_client):
    request = ServerReflectionRequest(list_services='')
    response = grpc_reflection_client.ServerReflectionInfo(iter([request]))
    res = await response.read()
    list_services = res.list_services_response
    services = list_services.service
    result = set([service.name for service in services])
    reference = set([
        'grpc.reflection.v1alpha.ServerReflection',
        'grpc.health.v1.Health',
    ])
    assert result == reference
