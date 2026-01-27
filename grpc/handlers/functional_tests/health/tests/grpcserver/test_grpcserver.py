try:
    from src.proto.grpc.health.v1 import health_pb2
    from src.proto.grpc.health.v1 import health_pb2_grpc
except ImportError:
    from health.v1 import health_pb2
    from health.v1 import health_pb2_grpc


async def test_say_hello(grpc_client: health_pb2_grpc.HealthStub):
    request = health_pb2.HealthCheckRequest()
    response: health_pb2.HealthCheckResponse = await grpc_client.Check(request)
    assert response.status == health_pb2.HealthCheckResponse.SERVING
