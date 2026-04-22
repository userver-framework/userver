import grpc
import pytest

import samples.greeter_pb2 as greeter_pb2


async def test_grpc_congestion_control_limit(grpc_client, service_client, testpoint):
    @testpoint('congestion-control-sensor')
    def tp_congestion_control_sensor(data):
        return {}

    @testpoint('congestion-control')
    def tp_congestion_control(data):
        return {'force-rps-limit': 0}

    @testpoint('congestion-control-apply')
    def tp_congestion_control_apply(data):
        return {}

    await service_client.enable_testpoints()

    # wait until server obtains the new limit, up to 1 second
    await tp_congestion_control.wait_call()
    await tp_congestion_control_apply.wait_call()

    # Random non-ping handler is throttled
    with pytest.raises(grpc.RpcError) as error:
        request = greeter_pb2.GreetingRequest(name='Python')
        await grpc_client.SayHello(request, wait_for_ready=True)
    assert error.value.details() == 'Congestion control: rate limit exceeded'
    assert error.value.code() == grpc.StatusCode.RESOURCE_EXHAUSTED

    # A hack to disable CC for other tests
    @testpoint('congestion-control')
    def tp_congestion_control_disable(data):
        return {'force-rps-limit': None}

    await tp_congestion_control_disable.wait_call()
    await tp_congestion_control_apply.wait_call()


async def test_grpc_congestion_control_activate(grpc_client, service_client, testpoint):
    @testpoint('congestion-control-sensor')
    def tp_congestion_control_sensor(data):
        return {
            'current-load': 100,
            'overload-events-count': 4,
            'no-overload-events-count': 96,
        }

    @testpoint('congestion-control')
    def tp_congestion_control(data):
        return {}

    @testpoint('congestion-control-apply')
    def tp_congestion_control_apply(data):
        return {}

    await service_client.enable_testpoints()

    # is_overloaded -> true
    await tp_congestion_control_sensor.wait_call()
    await tp_congestion_control.wait_call()
    await tp_congestion_control_apply.wait_call()

    # current_limit -> 1
    await tp_congestion_control_sensor.wait_call()
    await tp_congestion_control.wait_call()
    await tp_congestion_control_apply.wait_call()

    with pytest.raises(grpc.RpcError) as error:
        for i in range(0, 2):
            request = greeter_pb2.GreetingRequest(name=f'Name{i}/1')
            await grpc_client.SayHello(request, wait_for_ready=True)
    assert error.value.details() == 'Congestion control: rate limit exceeded'
    assert error.value.code() == grpc.StatusCode.RESOURCE_EXHAUSTED

    @testpoint('congestion-control-sensor')
    def tp_congestion_control_sensor_disable(data):
        return {
            'current-load': 100,
            'overload-events-count': 0,
            'no-overload-events-count': 100,
        }

    # current_limit -> 1
    await tp_congestion_control_sensor_disable.wait_call()
    await tp_congestion_control.wait_call()
    await tp_congestion_control_apply.wait_call()

    # current_limit -> null
    await tp_congestion_control_sensor_disable.wait_call()
    await tp_congestion_control.wait_call()
    await tp_congestion_control_apply.wait_call()

    for i in range(1, 10):
        request = greeter_pb2.GreetingRequest(name=f'Name{i}/2')
        response = await grpc_client.SayHello(request, wait_for_ready=True)
        assert response.greeting == f'Hello, Name{i}/2!'


async def test_grpc_cancellation(grpc_client, service_client, testpoint):
    @testpoint('testpoint_cancel')
    def cancel_testpoint(data):
        pass

    await service_client.enable_testpoints()

    with pytest.raises(grpc.RpcError) as error:
        request = greeter_pb2.GreetingRequest(name='test_payload_cancellation')
        await grpc_client.SayHello(request, wait_for_ready=True, timeout=0.1)
    assert error.value.details() == 'Deadline Exceeded'
    assert error.value.code() == grpc.StatusCode.DEADLINE_EXCEEDED

    await cancel_testpoint.wait_call()
