import grpc
import pytest
import samples.greeter_pb2 as greeter_pb2  # noqa: E402, E501

import requests_server


@pytest.mark.parametrize('case', requests_server.ALL_CASES)
async def test_basic(grpc_client, gate, case):
    await requests_server.check_ok_for(case, grpc_client, gate)


async def test_grpc_cc_enabled(grpc_client, service_client, testpoint):
    @testpoint('congestion-control')
    def tp_cc_enable(data):
        return {'force-rps-limit': 0}

    @testpoint('congestion-control-apply')
    def tp_cc_apply(data):
        return {}

    # wait until server obtains the new limit, up to 1 second
    await service_client.enable_testpoints()
    await tp_cc_enable.wait_call()
    await tp_cc_apply.wait_call()

    # Random non-ping handler is throttled
    with pytest.raises(grpc.RpcError) as error:
        request = greeter_pb2.GreetingRequest(name='Python')
        await grpc_client.SayHello(request, wait_for_ready=True)
    assert error.value.details() == 'Congestion control: rate limit exceeded'
    assert error.value.code() == grpc.StatusCode.RESOURCE_EXHAUSTED

    # A hack to disable CC for other tests
    @testpoint('congestion-control')
    def tp_cc_disable(data):
        return {}

    await tp_cc_disable.wait_call()
    await tp_cc_apply.wait_call()


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
