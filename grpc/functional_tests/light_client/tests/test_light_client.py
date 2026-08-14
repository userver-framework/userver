import grpc

import samples.greeter_pb2 as greeter_protos
import samples.greeter_pb2_grpc as greeter_services


async def test_regular_client(service_client, grpc_mockserver):
    @grpc_mockserver(greeter_services.GreeterServiceServicer.SayHello)
    async def mock_say_hello(request, context: grpc.aio.ServicerContext):
        return greeter_protos.GreetingResponse(greeting=f'Hello, {request.name}!')

    await service_client.run_task('call-say-hello-regular')
    assert mock_say_hello.times_called == 1


async def test_light_client(service_client, grpc_mockserver):
    @grpc_mockserver(greeter_services.GreeterServiceServicer.SayHello)
    async def mock_say_hello(request, context: grpc.aio.ServicerContext):
        return greeter_protos.GreetingResponse(greeting=f'Hello, {request.name}!')

    # The "light" client factory uses GetDefaultsAsConstantSource() and never
    # constructs grpc-client-common's shared default retry-limiter; this call
    # merely proves the service starts up and the client still works.
    await service_client.run_task('call-say-hello-light')
    assert mock_say_hello.times_called == 1


async def test_retry_limiter_create_counts(monitor_client):
    # Retry-limiters are created once, at client factory construction time
    # (not per-call), so these counts are stable regardless of test order.

    # The regular client factory falls back to grpc-client-common's shared
    # default retry-limiter (blocking-retry-limiter), so it's created once.
    regular = await monitor_client.single_metric(
        'blocking-retry-limiter.create-count.regular-greeter-client',
    )
    assert regular.value == 1

    # The light client factory skips that shared default entirely, so it's
    # never created for it.
    light = await monitor_client.single_metric(
        'blocking-retry-limiter.create-count.light-greeter-client',
    )
    assert light.value == 0

    # A light factory with an explicit retry-limiter override still honors
    # it, even though the grpc-client-common-level default is skipped. This
    # override uses a dedicated non-blocking-retry-limiter (not
    # blocking-retry-limiter) to avoid a startup deadlock — see
    # non_blocking_retry_limiter.hpp.
    light_explicit_override = await monitor_client.single_metric(
        'non-blocking-retry-limiter.create-count.light-greeter-client-explicit-retry-limiter',
    )
    assert light_explicit_override.value == 1

    # `retry-limiter: none` forces no retry-limiter for a regular (non-light)
    # client factory, even though grpc-client-common has a default
    # configured (blocking-retry-limiter) — this override is orthogonal to
    # use-constant-dynamic-configs.
    no_retry_limiter = await monitor_client.single_metric(
        'blocking-retry-limiter.create-count.regular-greeter-client-no-retry-limiter',
    )
    assert no_retry_limiter.value == 0
