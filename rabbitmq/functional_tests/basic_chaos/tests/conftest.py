import json
import typing

import pytest
from pytest_userver import chaos


pytest_plugins = ['pytest_userver.plugins.rabbitmq']


@pytest.fixture(scope='session', name='gate_settings')
def rabbitmq_gate_settings() -> typing.Tuple[str, int]:
    return ('localhost', 17329)


@pytest.fixture(scope='session')
def service_env(gate_settings) -> dict:
    secdist_config = {
        'rabbitmq_settings': {
            'chaos-rabbit-alias': {
                'hosts': [gate_settings[0]],
                'port': gate_settings[1],
                'login': 'guest',
                'password': 'guest',
                'vhost': '/',
            },
        },
    }

    return {'SECDIST_CONFIG': json.dumps(secdist_config)}


@pytest.fixture(scope='session')
async def _gate(loop, gate_settings, _rabbitmq_service_settings):
    connection_info = _rabbitmq_service_settings.get_connection_info()

    gate_config = chaos.GateRoute(
        name='rabbitmq proxy',
        host_for_client=gate_settings[0],
        port_for_client=gate_settings[1],
        host_to_server=connection_info.host,
        port_to_server=connection_info.tcp_port,
    )
    async with chaos.TcpGate(gate_config, loop) as proxy:
        yield proxy


@pytest.fixture(name='gate')
async def _gate_ready(service_client, _gate):
    _gate.to_client_pass()
    _gate.to_server_pass()
    _gate.start_accepting()

    await _gate.wait_for_connections(timeout=5.0)
    yield _gate
