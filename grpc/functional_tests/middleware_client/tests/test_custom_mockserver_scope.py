import pytest_userver.grpc

import samples.greeter_pb2 as greeter_protos
import samples.greeter_pb2_grpc as greeter_services


async def test_custom_inner_scope(grpc_mockserver, grpc_mockserver_session, service_client):
    """
    In this test, there are two Mockserver scopes: grpc_mockserver and a custom inner scope.
    When the inner scope is removed, the outer scope's mock should be left as a fallback.
    """

    @grpc_mockserver(greeter_services.GreeterServiceServicer.SayHello)
    async def outer_mock(request, context):
        return greeter_protos.GreetingResponse(greeting='Hello from outer mock!')

    await service_client.run_task('call-say-hello')
    assert outer_mock.times_called == 1

    with pytest_userver.grpc.Mockserver(
        mockserver_session=grpc_mockserver_session,
        experimental=True,
    ) as inner_mockserver:
        await service_client.run_task('call-say-hello')
        assert outer_mock.times_called == 2

        @inner_mockserver(greeter_services.GreeterServiceServicer.SayHello)
        async def inner_mock_v1(request, context):
            return greeter_protos.GreetingResponse(greeting='Hello from inner mock v1!')

        await service_client.run_task('call-say-hello')
        assert inner_mock_v1.times_called == 1

        @inner_mockserver(greeter_services.GreeterServiceServicer.SayHello)
        async def inner_mock_v2(request, context):
            return greeter_protos.GreetingResponse(greeting='Hello from inner mock v2!')

        await service_client.run_task('call-say-hello')
        assert inner_mock_v2.times_called == 1

    await service_client.run_task('call-say-hello')
    assert outer_mock.times_called == 3


async def test_competing_mockserver_scopes(grpc_mockserver_session, service_client):
    """
    In this test, there are two Mockserver scopes.
    Mocks for SayHello are set up in the order 1-2-1-2-1, with checks that the latest mock is used each time.
    Then Mockserver 1 is closed, which should remove its mocks, leaving the latest mock from 2 as a backup.
    """
    with pytest_userver.grpc.Mockserver(
        mockserver_session=grpc_mockserver_session,
        experimental=True,
    ) as first_mockserver:
        with pytest_userver.grpc.Mockserver(
            mockserver_session=grpc_mockserver_session,
            experimental=True,
        ) as second_mockserver:

            @first_mockserver(greeter_services.GreeterServiceServicer.SayHello)
            async def first_mock_v1(request, context):
                return greeter_protos.GreetingResponse(greeting='Hello from first mock v1!')

            await service_client.run_task('call-say-hello')
            assert first_mock_v1.times_called == 1

            @second_mockserver(greeter_services.GreeterServiceServicer.SayHello)
            async def second_mock_v1(request, context):
                return greeter_protos.GreetingResponse(greeting='Hello from second mock v1!')

            await service_client.run_task('call-say-hello')
            assert second_mock_v1.times_called == 1

            @first_mockserver(greeter_services.GreeterServiceServicer.SayHello)
            async def first_mock_v2(request, context):
                return greeter_protos.GreetingResponse(greeting='Hello from first mock v2!')

            await service_client.run_task('call-say-hello')
            assert first_mock_v2.times_called == 1

            @second_mockserver(greeter_services.GreeterServiceServicer.SayHello)
            async def second_mock_v2(request, context):
                return greeter_protos.GreetingResponse(greeting='Hello from second mock v2!')

            await service_client.run_task('call-say-hello')
            assert second_mock_v2.times_called == 1

            @first_mockserver(greeter_services.GreeterServiceServicer.SayHello)
            async def first_mock_v3(request, context):
                return greeter_protos.GreetingResponse(greeting='Hello from first mock v3!')

            await service_client.run_task('call-say-hello')
            assert first_mock_v3.times_called == 1

            first_mockserver.reset_mocks()

            await service_client.run_task('call-say-hello')
            assert second_mock_v2.times_called == 2
