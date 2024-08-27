from samples import greeter_pb2


HEADERS_TO_PROPAGATE = ['custom-header-1', 'custom-header-2']
HEADERS_NOT_TO_PROPAGATE = [
    'custom-header-1-non-prop',
    'custom-header-2-non-prop',
]


async def test_propagation_headers_mixed_from_grpc(grpc_client, mockserver):
    headers = {
        HEADERS_NOT_TO_PROPAGATE[0]: '1',
        HEADERS_NOT_TO_PROPAGATE[1]: '2',
        HEADERS_TO_PROPAGATE[0]: '3',
        HEADERS_TO_PROPAGATE[1]: '4',
    }

    @mockserver.json_handler('/test-service/echo-no-body')
    async def nobody_handler(request):
        assert (
            request.headers[HEADERS_TO_PROPAGATE[0]]
            == headers[HEADERS_TO_PROPAGATE[0]]
        )
        assert (
            request.headers[HEADERS_TO_PROPAGATE[1]]
            == headers[HEADERS_TO_PROPAGATE[1]]
        )
        assert HEADERS_NOT_TO_PROPAGATE[0] not in request.headers
        assert HEADERS_NOT_TO_PROPAGATE[1] not in request.headers
        return mockserver.make_response()

    metadata = []
    for key, value in headers.items():
        metadata.append((key, value))
    request = greeter_pb2.GreetingRequest(name='Python')
    response = await grpc_client.CallEchoNobody(request, metadata=metadata)
    assert response.greeting == 'Call Echo Nobody'
    assert nobody_handler.times_called == 1
