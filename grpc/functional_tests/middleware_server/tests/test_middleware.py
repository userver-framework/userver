import asyncio

import samples.greeter_pb2 as greeter_protos  # noqa: E402, E501

MD_ONE_REQ = ' One'
MD_TWO_REQ = ' Two'
MD_ONE_RES = ' EndOne'
MD_TWO_RES = ' EndTwo'


async def test_say_hello(grpc_client):
    request = greeter_protos.GreetingRequest(name='Python')
    response = await grpc_client.SayHello(request)
    assert (
        response.greeting
        == f'Hello, Python{MD_ONE_REQ}{MD_TWO_REQ}!{MD_TWO_RES}{MD_ONE_RES}'
    )


async def test_say_hello_response_stream(grpc_client):
    request = greeter_protos.GreetingRequest(name='C++')
    r = '!'
    async for response in grpc_client.SayHelloResponseStream(
        request, wait_for_ready=True,
    ):
        assert (
            response.greeting
            == f'Hello, C++{MD_ONE_REQ}{MD_TWO_REQ}{r}{MD_TWO_RES}{MD_ONE_RES}'
        )
        r += '!'


async def _prepare_requests(names, sleep=1):
    reqs = []
    for name in names:
        reqs.append(greeter_protos.GreetingRequest(name=name))
    for req in reqs:
        await asyncio.sleep(sleep)
        yield req


async def test_say_hello_request_stream(grpc_client):
    stream = await grpc_client.SayHelloRequestStream(
        _prepare_requests(['Python', '!', '!'], 1), wait_for_ready=True,
    )
    assert (
        stream.greeting
        == 'Hello, Python One Two! One Two! One Two EndTwo EndOne'
    )


async def test_say_hello_streams(grpc_client):
    start = f'Python{MD_ONE_REQ}{MD_TWO_REQ}'
    end = f'{MD_TWO_RES}{MD_ONE_RES}'
    async for response in grpc_client.SayHelloStreams(
        _prepare_requests(['Python', '!', '!', '!'], 1), wait_for_ready=True,
    ):
        assert response.greeting == f'Hello, {start}{end}'
        start += f'!{MD_ONE_REQ}{MD_TWO_REQ}'
