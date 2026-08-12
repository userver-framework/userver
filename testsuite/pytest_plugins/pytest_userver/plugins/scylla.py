"""
Plugin that waits for a ScyllaDB cluster to become reachable and adjusts
the `scylla-*` components of the static config to point at it.
"""

# pylint: disable=redefined-outer-name
import dataclasses
import logging

import pytest

from testsuite.environment import utils as env_utils

pytest_plugins = ['pytest_userver.plugins.core']

USERVER_CONFIG_HOOKS = ['userver_config_scylla']

logger = logging.getLogger(__name__)

DEFAULT_HOST = 'localhost'
DEFAULT_CQL_PORT = 9042
DEFAULT_WAIT_TIMEOUT = 60.0

_SCYLLA_COMPONENT_PREFIX = 'scylla-'


def pytest_addoption(parser) -> None:
    group = parser.getgroup('scylla')
    group.addoption(
        '--scylla-host',
        help=(
            'Host of the ScyllaDB cluster used by functional tests. Defaults to $TESTSUITE_SCYLLA_HOST or "localhost".'
        ),
    )
    group.addoption(
        '--scylla-port',
        type=int,
        help=('CQL native transport port. Defaults to $TESTSUITE_SCYLLA_PORT or 9042.'),
    )
    group.addoption(
        '--scylla-wait-timeout',
        type=float,
        help=(
            'Seconds to wait for the CQL port to start accepting '
            'connections before failing service start. '
            'Defaults to $TESTSUITE_SCYLLA_WAIT_TIMEOUT or 60.'
        ),
    )


@dataclasses.dataclass(frozen=True)
class ConnectionInfo:
    host: str
    port: int

    def contact_points(self) -> str:
        if self.port == DEFAULT_CQL_PORT:
            return self.host
        return f'{self.host}:{self.port}'


@pytest.fixture(scope='session')
def scylla_connection_info(pytestconfig) -> ConnectionInfo:
    """
    Where the ScyllaDB cluster is expected to be running.

    @ingroup userver_testsuite_fixtures
    """
    host = pytestconfig.option.scylla_host or env_utils.getenv_str(
        'TESTSUITE_SCYLLA_HOST',
        DEFAULT_HOST,
    )
    port = pytestconfig.option.scylla_port or env_utils.getenv_int(
        'TESTSUITE_SCYLLA_PORT',
        DEFAULT_CQL_PORT,
    )
    return ConnectionInfo(host=host, port=port)


@pytest.fixture(scope='session')
def scylla_wait_timeout(pytestconfig) -> float:
    """@ingroup userver_testsuite_fixtures"""
    return pytestconfig.option.scylla_wait_timeout or env_utils.getenv_float(
        'TESTSUITE_SCYLLA_WAIT_TIMEOUT',
        DEFAULT_WAIT_TIMEOUT,
    )


@pytest.fixture(scope='session')
def _scylla_tcp_ready(scylla_connection_info, scylla_wait_timeout) -> None:
    info = scylla_connection_info
    logger.info(
        'Waiting up to %.1fs for ScyllaDB CQL port at %s:%d',
        scylla_wait_timeout,
        info.host,
        info.port,
    )
    if not env_utils.wait_tcp_connection(
        host=info.host,
        port=info.port,
        timeout=scylla_wait_timeout,
    ):
        raise RuntimeError(
            f'ScyllaDB is not reachable at {info.host}:{info.port} after '
            f'{scylla_wait_timeout:.1f}s. Start the cluster or set '
            '--scylla-host / TESTSUITE_SCYLLA_HOST to a reachable endpoint.',
        )


@pytest.fixture
def scylla(_scylla_tcp_ready, scylla_connection_info) -> ConnectionInfo:
    """@ingroup userver_testsuite_fixtures"""
    return scylla_connection_info


@pytest.fixture(scope='session')
def userver_config_scylla(scylla_connection_info, _scylla_tcp_ready):
    """@ingroup userver_testsuite_fixtures"""
    contact_points = scylla_connection_info.contact_points()

    def _patch(config_yaml, config_vars):
        components = config_yaml['components_manager']['components']
        for name, params in components.items():
            if not isinstance(params, dict):
                continue
            if not name.startswith(_SCYLLA_COMPONENT_PREFIX):
                continue
            if 'dbalias' in params and params['dbalias']:
                continue
            if 'dbconnection' not in params:
                continue
            params['dbconnection'] = contact_points

    return _patch
