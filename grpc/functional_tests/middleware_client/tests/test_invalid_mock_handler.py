import re
import traceback

import google.protobuf.empty_pb2 as empty_protos
import pytest
import pytest_userver.client

import samples.greeter_pb2 as greeter_protos
import samples.greeter_pb2_grpc as greeter_services

SAY_HELLO = 'samples.api.GreeterService/SayHello'


# Overriding testsuite fixture
@pytest.fixture(name='asyncexc_check')
def _asyncexc_check(asyncexc_check):
    """
    We've inserted testsuite's asyncexc_check after calls to the service client, but for the current tests these checks
    are an impediment, as they would hide the errors thrown from ugrpc client.

    Disable auto-check of background errors by default for tests in this file..
    """

    def check(*, really_check: bool = False):
        if really_check:
            asyncexc_check()

    return check


def full_exception_message(exc: BaseException) -> str:
    return ''.join(traceback.format_exception(type(exc), exc, exc.__traceback__))


async def test_ok(grpc_mockserver):
    """
    This test is needed to ensure that `GrpcServiceMock` is created in `test_missing`.
    So we will test a situation when `grpc_mockserver` knows about the service, but there is no mocked handler.
    """

    @grpc_mockserver(greeter_services.GreeterServiceServicer.SayHello)
    async def mock_say_hello(request, context):
        return greeter_protos.GreetingResponse()


async def test_missing(service_client, asyncexc_check):
    message = 'Method not found!'
    expected_error = re.compile(f"'{SAY_HELLO}' failed: code=UNIMPLEMENTED, message='{message}'")

    with pytest.raises(pytest_userver.client.TestsuiteTaskFailed, match=expected_error):
        await service_client.run_task('call-say-hello')

    # Even if the service swallows the error from the client and keeps going, the test will still fail
    # through testsuite's asyncexc mechanism.
    with pytest.raises(Exception) as ex_info:
        asyncexc_check(really_check=True)
    expected_error = (
        f'testsuite.mockserver.exceptions.HandlerNotFoundError: '
        f"gRPC mockserver handler is not installed for '{SAY_HELLO}'."
    )
    assert expected_error in full_exception_message(ex_info.value)


async def test_assert(service_client, grpc_mockserver, asyncexc_check):
    @grpc_mockserver(greeter_services.GreeterServiceServicer.SayHello)
    async def mock_say_hello(request: greeter_protos.GreetingRequest, context):
        assert request.name == 'expected'

    expected_error = re.compile(f"'{SAY_HELLO}' failed: code=UNKNOWN, message='Unexpected <class 'AssertionError'>")

    with pytest.raises(pytest_userver.client.TestsuiteTaskFailed, match=expected_error):
        await service_client.run_task('call-say-hello')

    # Even if the service swallows the error from the client and keeps going, the test will still fail
    # through testsuite's asyncexc mechanism.
    with pytest.raises(Exception) as ex_info:
        asyncexc_check(really_check=True)
    assert 'AssertionError:' in full_exception_message(ex_info.value)


async def test_exception(service_client, grpc_mockserver, asyncexc_check):
    @grpc_mockserver(greeter_services.GreeterServiceServicer.SayHello)
    async def mock_say_hello(request, context):
        raise ValueError('Something went wrong in the mocked handler')

    message = "Unexpected <class 'ValueError'>: Something went wrong in the mocked handler"
    expected_error = re.compile(f"'{SAY_HELLO}' failed: code=UNKNOWN, message='{message}'")

    with pytest.raises(Exception, match=expected_error):
        await service_client.run_task('call-say-hello')

    # Even if the service swallows the error from the client and keeps going, the test will still fail
    # through testsuite's asyncexc mechanism.
    with pytest.raises(Exception) as ex_info:
        asyncexc_check(really_check=True)
    assert 'ValueError: Something went wrong in the mocked handler' in full_exception_message(ex_info.value)


async def test_wrong_response_type_non_message(service_client, grpc_mockserver, asyncexc_check):
    @grpc_mockserver(greeter_services.GreeterServiceServicer.SayHello)
    async def mock_say_hello(request, context):
        return 42

    message = '.*Expected a protobuf Message response, got: 42 \\(int\\)'
    expected_error = re.compile(f"'{SAY_HELLO}' failed: code=UNKNOWN, message='{message}'")

    with pytest.raises(pytest_userver.client.TestsuiteTaskFailed, match=expected_error):
        await service_client.run_task('call-say-hello')

    # Even if the service swallows the error from the client and keeps going, the test will still fail
    # through testsuite's asyncexc mechanism.
    with pytest.raises(Exception) as ex_info:
        asyncexc_check(really_check=True)
    expected_message = (
        f'ValueError: In grpc_mockserver handler for {SAY_HELLO}: Expected a protobuf Message response, got: 42 (int)'
    )
    assert expected_message in full_exception_message(ex_info.value)


async def test_wrong_response_type_message(service_client, grpc_mockserver, asyncexc_check):
    @grpc_mockserver(greeter_services.GreeterServiceServicer.SayHello)
    async def mock_say_hello(request, context):
        return empty_protos.Empty()

    message = '.*Expected a response of type "samples.api.GreetingResponse", got: "google.protobuf.Empty"'
    expected_error = re.compile(f"'{SAY_HELLO}' failed: code=UNKNOWN, message='{message}'")

    with pytest.raises(pytest_userver.client.TestsuiteTaskFailed, match=expected_error):
        await service_client.run_task('call-say-hello')

    # Even if the service swallows the error from the client and keeps going, the test will still fail
    # through testsuite's asyncexc mechanism.
    with pytest.raises(Exception) as ex_info:
        asyncexc_check(really_check=True)
    expected_message = (
        f'ValueError: In grpc_mockserver handler for {SAY_HELLO}: '
        'Expected a response of type "samples.api.GreetingResponse", got: "google.protobuf.Empty"'
    )
    assert expected_message in full_exception_message(ex_info.value)
