import healthchecking.healthchecking_pb2 as healthchecking_pb2


async def test_say_hello(grpc_client):
    request = healthchecking_pb2.HealthCheckRequest()
    response = await grpc_client.Check(request)
    assert response.status == 1  # Don't know how to get enum value
