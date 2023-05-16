async def test_say_hello(grpc_client, healthchecking_proto, gate):
    request = healthchecking_proto.HealthCheckRequest()
    response = await grpc_client.Check(request)
    assert response.status == 1  # Don't know how to get enum value
