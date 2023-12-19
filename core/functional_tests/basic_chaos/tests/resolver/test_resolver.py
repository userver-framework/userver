import asyncio
import enum
import typing

import pytest


SUCCESS_IPV4 = '77.88.55.55:0'
SUCCESS_IPV6 = '[2a02:6b8:a::a]:0'
SUCCESS_RESOLVE = f'{SUCCESS_IPV6}, {SUCCESS_IPV4}'

DEFAULT_TIMEOUT = 15


class CheckQuery(enum.Enum):
    FROM_MOCK = 1
    FROM_CACHE = 2
    NO_CHECK = 3


@pytest.fixture(name='call')
def _call(service_client, gen_domain_name, dns_mock_stats):
    async def _call(
            resolve: typing.Optional[str] = None,
            htype: str = 'resolve',
            check_query: CheckQuery = CheckQuery.NO_CHECK,
            **args,
    ):
        if not resolve:
            resolve = gen_domain_name()

        dns_times_called = dns_mock_stats.get_stats()

        response = await service_client.get(
            '/chaos/resolver',
            params={'type': htype, 'host_to_resolve': resolve, **args},
        )

        if check_query == CheckQuery.FROM_MOCK:
            # Could be two different reqeuests for IPv4 and IPv6,
            # or a single one, or some retries.
            assert dns_mock_stats.get_stats() > dns_times_called
            queries = dns_mock_stats.get_queries()
            assert resolve in queries[-dns_times_called:]
        elif check_query == CheckQuery.FROM_CACHE:
            assert dns_mock_stats.get_stats() == dns_times_called
        elif check_query == CheckQuery.NO_CHECK:
            pass

        return response

    return _call


@pytest.fixture(name='flush_resolver_cache')
def _flush_resolver_cache(service_client):
    async def _flush_resolver_cache():
        result = await service_client.get(
            '/chaos/resolver', params={'type': 'flush'},
        )

        assert result.status == 200
        assert result.text == 'flushed'

    return _flush_resolver_cache


@pytest.fixture(autouse=True)
async def _do_flush(flush_resolver_cache):
    await flush_resolver_cache()


@pytest.fixture(name='check_restore')
def _check_restore(gate, call, flush_resolver_cache, gen_domain_name):
    async def _do_restore(domain: typing.Optional[str] = None):
        gate.to_server_pass()
        gate.to_client_pass()

        if not domain:
            domain = gen_domain_name()

        response = await call(
            resolve=domain,
            check_query=CheckQuery.FROM_MOCK,
            timeout=DEFAULT_TIMEOUT,
        )

        assert response.status == 200
        assert response.text == SUCCESS_RESOLVE

    return _do_restore


@pytest.fixture(autouse=True)
async def _auto_check_restore(request, loop, check_restore):
    def teardown():
        loop.run_until_complete(check_restore())

    request.addfinalizer(teardown)


async def test_ok(call, gate):
    response = await call()

    assert response.status == 200
    assert response.text == SUCCESS_RESOLVE


@pytest.mark.skip(reason='corrupted data can still be valid')
async def test_corrupt_data(call, gate, check_restore):
    gate.to_client_corrupt_data()
    response = await call()

    assert response.status == 500
    assert not response.text


async def test_cached_name(call, gate, check_restore, gen_domain_name):
    # make a request to cache the result
    name = gen_domain_name()
    response = await call(resolve=name, check_query=CheckQuery.FROM_MOCK)
    assert response.status == 200
    assert response.text == SUCCESS_RESOLVE

    gate.to_client_noop()

    response = await call(resolve=name, check_query=CheckQuery.FROM_CACHE)
    assert response.status == 200
    assert response.text == SUCCESS_RESOLVE


async def test_noop(call, gate, check_restore, gen_domain_name):
    gate.to_client_noop()

    response = await call(check_query=CheckQuery.FROM_MOCK)

    assert response.status == 500
    assert not response.text


async def test_delay(call, gate, check_restore):
    gate.to_client_delay(delay=10)

    response = await call(timeout=1)

    assert response.status == 500
    assert not response.text


async def test_close_on_data(call, gate, check_restore):
    gate.to_client_close_on_data()

    response = await call()

    assert response.status == 500
    assert not response.text

    await gate.stop()
    gate.start()


async def test_limit_bytes(call, gate, check_restore):
    gate.to_client_limit_bytes(10)
    # 10 bytes less than dns-mock message, so part of this message will be
    # dropped and it must lead error

    response = await call()

    assert response.status == 500
    assert not response.text

    await gate.stop()
    gate.start()


async def test_dns_switch(
        service_client, gate, check_restore, dns_mock2_lazy, gen_domain_name,
):
    await gate.stop()

    async with dns_mock2_lazy as server:
        params = {
            'type': 'resolve',
            'host_to_resolve': gen_domain_name(),
            'timeout': 30,
        }
        response = await service_client.get('/chaos/resolver', params=params)
        assert server.get_stats() > 0
        assert response.status == 200
        assert SUCCESS_IPV4 in response.text or SUCCESS_IPV6 in response.text

    gate.start()


async def test_dns_switch_on_in_flight_request(
        service_client,
        gate,
        check_restore,
        dns_mock2_lazy,
        gen_domain_name,
        testpoint,
):
    await gate.stop()

    params = {
        'type': 'resolve',
        'host_to_resolve': gen_domain_name(),
        'timeout': 10,  # less than network timeout of the resolver
    }

    @testpoint('net-resolver')
    def _net_resolve_testpoint(data):
        assert data['status'] == 0

    async with dns_mock2_lazy:
        response = await service_client.get('/chaos/resolver', params=params)
        assert response.status != 200

    gate.start()

    response = await service_client.get('/chaos/resolver', params=params)
    assert response.status == 200


async def test_dns_switch_small_timeout(
        service_client,
        gate,
        check_restore,
        dns_mock2_lazy,
        gen_domain_name,
        testpoint,
):
    await gate.stop()

    params = {
        'type': 'resolve',
        'host_to_resolve': gen_domain_name(),
        'timeout': 10,  # less than network timeout of the resolver
    }

    @testpoint('net-resolver')
    def _net_resolve_testpoint(data):
        assert data['status'] == 0

    async with dns_mock2_lazy as server:
        # First resolve attempt fails due to a small timeout and irresponsive
        # first DNS server.
        response = await service_client.get('/chaos/resolver', params=params)
        assert response.status != 200

        # Should succeed, because resolving continues in background.
        await _net_resolve_testpoint.wait_call()

        response = await service_client.get('/chaos/resolver', params=params)
        assert server.get_stats() > 0
        assert response.status == 200
        assert SUCCESS_IPV4 in response.text or SUCCESS_IPV6 in response.text

    gate.start()


async def test_dns_switch_erefused(
        service_client, check_restore, dns_mock, gen_domain_name, testpoint,
):
    last_resolve_status = 0

    @testpoint('net-resolver')
    def _net_resolve_testpoint(data):
        nonlocal last_resolve_status
        last_resolve_status = data['status']

    async def _request_and_wait_for_update():
        params = {'type': 'resolve', 'host_to_resolve': gen_domain_name()}
        done, _ = await asyncio.wait(
            {
                asyncio.create_task(
                    service_client.get('/chaos/resolver', params=params),
                    name='request',
                ),
                asyncio.create_task(
                    _net_resolve_testpoint.wait_call(
                        timeout=DEFAULT_TIMEOUT + 5.0,
                    ),
                ),
            },
            return_when=asyncio.ALL_COMPLETED,
        )
        for task in done:
            if task.get_name() == 'request':
                return task.result()

    dns_mock.set_refuse_responses()
    response = await _request_and_wait_for_update()
    dns_mock.set_ok_responses()

    assert response.text == ''

    on_refuse_status = last_resolve_status
    # ARES_ECONNREFUSED(11) and ARES_EREFUSED(6) statuses are the most common
    # ones. Depending on the c-ares version ARES_ETIMEOUT(12) and other errors
    # could be reported.
    assert on_refuse_status != 0

    response = await _request_and_wait_for_update()
    assert response.status == 200, (
        f'{response.status} != 200 after '
        f'on_refuse_status={on_refuse_status}'
    )
    assert response.text == SUCCESS_RESOLVE, (
        f'Unexpected {response.text} after '
        f'on_refuse_status={on_refuse_status}'
    )

    assert last_resolve_status == 0, (
        f'Unexpected resolve status {last_resolve_status} after '
        f'on_refuse_status={on_refuse_status}'
    )
