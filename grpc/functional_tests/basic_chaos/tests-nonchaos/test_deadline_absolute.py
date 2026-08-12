import datetime

import grpc
import pytest

import samples.greeter_pb2 as greeter_pb2
import samples.greeter_pb2_grpc as greeter_pb2_grpc

DP_ABSOLUTE_DEADLINE = 'x-request-deadline'

REQUEST = greeter_pb2.GreetingRequest(name='Python')


def _make_deadline_epoch_us(offset_seconds: float) -> str:
    deadline_utc = datetime.datetime.now(datetime.timezone.utc) + datetime.timedelta(
        seconds=offset_seconds,
    )
    unix_epoch_utc = datetime.datetime(1970, 1, 1, tzinfo=datetime.timezone.utc)
    one_microsecond = datetime.timedelta(microseconds=1)
    microseconds_since_epoch = (deadline_utc - unix_epoch_utc) // one_microsecond
    return str(microseconds_since_epoch)


async def test_absolute_deadline_used(grpc_client):
    response = await grpc_client.SayHello(
        REQUEST,
        metadata=[(DP_ABSOLUTE_DEADLINE, _make_deadline_epoch_us(120.0))],
    )
    assert response.greeting == 'Hello, Python!'


async def test_absolute_deadline_expired(grpc_client):
    with pytest.raises(grpc.RpcError) as error:
        await grpc_client.SayHello(
            REQUEST,
            metadata=[(DP_ABSOLUTE_DEADLINE, _make_deadline_epoch_us(-120.0))],
        )
    assert error.value.code() == grpc.StatusCode.DEADLINE_EXCEEDED


@pytest.mark.config(USERVER_DEADLINE_PROPAGATION_ABSOLUTE_TIMESTAMP_ENABLED=False)
async def test_absolute_deadline_disabled_dynamically(grpc_client):
    response = await grpc_client.SayHello(
        REQUEST,
        metadata=[(DP_ABSOLUTE_DEADLINE, _make_deadline_epoch_us(-1.0))],
        timeout=5.0,
    )
    assert response.greeting == 'Hello, Python!'


async def test_absolute_deadline_clock_skew_fallback(grpc_client):
    response = await grpc_client.SayHello(
        REQUEST,
        metadata=[(DP_ABSOLUTE_DEADLINE, _make_deadline_epoch_us(5.0 + 120.0))],
        timeout=5.0,
    )
    assert response.greeting == 'Hello, Python!'


async def test_absolute_deadline_clock_skew_fallback_when_negative_skew(grpc_client):
    response = await grpc_client.SayHello(
        REQUEST,
        metadata=[(DP_ABSOLUTE_DEADLINE, _make_deadline_epoch_us(-120.0))],
        timeout=5.0,
    )
    assert response.greeting == 'Hello, Python!'


@pytest.mark.config(USERVER_DEADLINE_PROPAGATION_CLOCK_SKEW_THRESHOLD_MS=0)
async def test_absolute_deadline_threshold_zero_disables_skew_check(
    grpc_client,
):
    with pytest.raises(grpc.RpcError) as error:
        await grpc_client.SayHello(
            REQUEST,
            metadata=[(DP_ABSOLUTE_DEADLINE, _make_deadline_epoch_us(-120.0))],
        )
    assert error.value.code() == grpc.StatusCode.DEADLINE_EXCEEDED


async def test_absolute_deadline_invalid_format(grpc_client):
    response = await grpc_client.SayHello(
        REQUEST,
        metadata=[(DP_ABSOLUTE_DEADLINE, 'not-a-timestamp')],
        timeout=5.0,
    )
    assert response.greeting == 'Hello, Python!'


async def test_absolute_deadline_propagated_as_is(grpc_client, grpc_mockserver):
    epoch_us_deadline = _make_deadline_epoch_us(120.0)
    captured_metadata = None

    @grpc_mockserver(greeter_pb2_grpc.GreeterServiceServicer.SayHello)
    async def mock_say_hello(request, context: grpc.aio.ServicerContext):
        nonlocal captured_metadata
        captured_metadata = dict(context.invocation_metadata())
        return greeter_pb2.GreetingResponse(
            greeting='Hello, Python!',
        )

    response = await grpc_client.SayHello(
        greeter_pb2.GreetingRequest(name='test_deadline_propagation'),
        metadata=[(DP_ABSOLUTE_DEADLINE, epoch_us_deadline)],
        timeout=5.0,
    )
    assert response.greeting == 'Hello, Python!'
    assert captured_metadata is not None
    assert captured_metadata.get(DP_ABSOLUTE_DEADLINE) == epoch_us_deadline


async def test_absolute_deadline_synthesized_when_only_duration_propagated(
    grpc_client,
    grpc_mockserver,
):
    captured_metadata = None

    @grpc_mockserver(greeter_pb2_grpc.GreeterServiceServicer.SayHello)
    async def mock_say_hello(request, context: grpc.aio.ServicerContext):
        nonlocal captured_metadata
        captured_metadata = dict(context.invocation_metadata())
        return greeter_pb2.GreetingResponse(
            greeting='Hello, Python!',
        )

    response = await grpc_client.SayHello(
        greeter_pb2.GreetingRequest(name='test_deadline_propagation'),
        timeout=5.0,
    )
    assert response.greeting == 'Hello, Python!'
    assert captured_metadata is not None
    assert DP_ABSOLUTE_DEADLINE in captured_metadata
