# pylint: disable=invalid-name,unused-variable
import asyncio

import samples.greeter_pb2 as greeter_pb2  # noqa: E402, E501
import samples.greeter_pb2_grpc as greeter_pb2_grpc  # noqa: E402, E501


class GreeterService(greeter_pb2_grpc.GreeterServiceServicer):
    async def SayHello(self, request, context):
        return greeter_pb2.GreetingResponse(greeting=f'Hello, {request.name}!')

    async def SayHelloResponseStream(self, request, context):
        time_interval = 0.2
        message = f'Hello, {request.name}'
        for i in range(5):
            message += '!'
            await asyncio.sleep(time_interval)
            yield greeter_pb2.GreetingResponse(greeting=message)
        return

    async def SayHelloRequestStream(self, request_iterator, context):
        message = 'Hello, '
        async for request in request_iterator:
            message += f'{request.name}'
        return greeter_pb2.GreetingResponse(greeting=message)

    async def SayHelloStreams(self, request_iterator, context):
        time_interval = 0.2
        income = ''
        async for request in request_iterator:
            income += f'{request.name}'
            await asyncio.sleep(time_interval)
            yield greeter_pb2.GreetingResponse(greeting=f'Hello, {income}')
        return

    async def SayHelloIndependentStreams(self, request_iterator, context):
        time_interval_read = 0.2
        time_interval_write = 0.3
        final_string = ''
        async for request in request_iterator:
            await asyncio.sleep(time_interval_read)
            final_string += f'{request.name}'
        values = [
            'Python',
            'C++',
            'linux',
            'userver',
            'grpc',
            'kernel',
            'developer',
            'core',
            'anonim',
            'user',
        ]
        for val in values:
            await asyncio.sleep(time_interval_write)
            yield greeter_pb2.GreetingResponse(greeting=f'Hello, {val}')
        yield greeter_pb2.GreetingResponse(greeting=final_string)
