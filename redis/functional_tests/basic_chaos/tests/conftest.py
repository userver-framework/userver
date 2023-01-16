import json
import typing

import pytest

from pytest_userver import chaos

from testsuite.databases.redis import service


pytest_plugins = ['pytest_userver.plugins.redis']


@pytest.fixture(scope='session')
def sentinel_gate_settings() -> typing.Tuple[str, int]:
    return ('localhost', 27379)


@pytest.fixture(scope='session')
def master_gate_settings() -> typing.Tuple[str, int]:
    return ('localhost', 17379)


@pytest.fixture(scope='session')
def service_env(
        sentinel_gate_settings,
        _redis_service_settings: service.ServiceSettings,
):
    secdist_config = {
        'redis_settings': {
            'test': {
                'password': '',
                'sentinels': [
                    {
                        'host': sentinel_gate_settings[0],
                        'port': sentinel_gate_settings[1],
                    },
                ],
                'shards': [{'name': 'test_master0'}],
            },
        },
    }
    return {'SECDIST_CONFIG': json.dumps(secdist_config)}


@pytest.fixture(scope='session')
async def _sentinel_gate(
        loop,
        sentinel_gate_settings,
        _redis_service_settings: service.ServiceSettings,
):
    gate_config = chaos.GateRoute(
        name='sentinel proxy',
        host_for_client=sentinel_gate_settings[0],
        port_for_client=sentinel_gate_settings[1],
        host_to_server=_redis_service_settings.host,
        port_to_server=_redis_service_settings.sentinel_port,
    )
    async with chaos.TcpGate(gate_config, loop) as proxy:
        yield proxy


@pytest.fixture(name='sentinel_gate')
async def _sentinel_gate_ready(
        service_client,
        _sentinel_gate,
        _master_gate,
        _redis_service_settings: service.ServiceSettings,
):
    port = str(_redis_service_settings.master_ports[0])
    ptrn = r'\r\nip\r\n\$\d+\r\n\S+\r\n\$4\r\nport\r\n\$%d\r\n%s\r\n' % (
        len(port),
        port,
    )

    gate = _master_gate.get_sockname_for_clients()
    repl = '\r\nip\r\n$%d\r\n%s\r\n$4\r\nport\r\n$%d\r\n%d\r\n' % (
        len(gate[0]),
        gate[0],
        len(str(gate[1])),
        gate[1],
    )

    _sentinel_gate.to_client_substitute(ptrn, repl)
    _sentinel_gate.to_server_pass()
    _sentinel_gate.start_accepting()

    await _sentinel_gate.wait_for_connections(timeout=5.0)
    yield _sentinel_gate


@pytest.fixture(scope='session')
async def _master_gate(
        loop,
        master_gate_settings,
        _redis_service_settings: service.ServiceSettings,
):
    gate_config = chaos.GateRoute(
        name='master proxy',
        host_for_client=master_gate_settings[0],
        port_for_client=master_gate_settings[1],
        host_to_server=_redis_service_settings.host,
        port_to_server=_redis_service_settings.master_ports[0],
    )
    async with chaos.TcpGate(gate_config, loop) as proxy:
        yield proxy


@pytest.fixture(name='gate')
async def _master_gate_ready(service_client, _master_gate):
    _master_gate.to_client_pass()
    _master_gate.to_server_pass()
    _master_gate.start_accepting()

    await _master_gate.wait_for_connections(timeout=5.0)
    yield _master_gate
