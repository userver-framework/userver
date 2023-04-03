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


@pytest.fixture
def call(service_client, gen_domain_name, dns_mock_stats):
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
            # Plus two different reqeuests for IPv4 and IPv6
            assert dns_mock_stats.get_stats() == dns_times_called + 2
            queries = dns_mock_stats.get_queries()
            assert queries[-1] == resolve
            assert queries[-2] == resolve
        elif check_query == CheckQuery.FROM_CACHE:
            assert dns_mock_stats.get_stats() == dns_times_called
        elif check_query == CheckQuery.NO_CHECK:
            pass

        return response

    return _call


@pytest.fixture
def flush_resolver_cache(service_client):
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


@pytest.fixture
def check_restore(gate, call, flush_resolver_cache, gen_domain_name):
    async def _check_restore(domain: typing.Optional[str] = None):
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

    return _check_restore


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
