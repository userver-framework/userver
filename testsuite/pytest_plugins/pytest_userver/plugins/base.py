"""
Configure the service in testsuite.
"""

import pathlib
import random
import socket

import pytest

USERVER_CONFIG_HOOKS = ['userver_base_prepare_service_config']


def pytest_addoption(parser) -> None:
    group = parser.getgroup('userver')
    group.addoption(
        '--build-dir',
        type=pathlib.Path,
        help='Path to service build directory.',
    )

    group = parser.getgroup('Test service')
    group.addoption(
        '--service-binary',
        type=pathlib.Path,
        help='Path to service binary.',
    )
    group.addoption(
        '--service-port',
        help=('Main HTTP port of the service ' '(default: use the port from the static config)'),
        default=None,
        type=int,
    )
    group.addoption(
        '--monitor-port',
        help=('Monitor HTTP port of the service ' '(default: use the port from the static config)'),
        default=None,
        type=int,
    )
    group.addoption(
        '--service-source-dir',
        type=pathlib.Path,
        help='Path to service source directory.',
        default=pathlib.Path('.'),
    )


def pytest_configure(config):
    config.option.asyncio_mode = 'auto'


@pytest.fixture(scope='session')
def service_source_dir(pytestconfig) -> pathlib.Path:
    """
    Returns the path to the service source directory that is set by command
    line `--service-source-dir` option.

    Override this fixture to change the way the path to the service
    source directory is detected by testsuite.

    @ingroup userver_testsuite_fixtures
    """
    return pytestconfig.option.service_source_dir


@pytest.fixture(scope='session')
def build_dir(pytestconfig) -> pathlib.Path:
    """
    Returns the build directory set by command line `--build-dir` option.

    Override this fixture to change the way the build directory is
    detected by the testsuite.

    @ingroup userver_testsuite_fixtures
    """
    return pytestconfig.option.build_dir


@pytest.fixture(scope='session')
def service_binary(pytestconfig) -> pathlib.Path:
    """
    Returns the path to service binary set by command line `--service-binary`
    option.

    Override this fixture to change the way the path to service binary is
    detected by the testsuite.

    @ingroup userver_testsuite_fixtures
    """
    return pytestconfig.option.service_binary


@pytest.fixture(scope='session')
def service_port(pytestconfig, _original_service_config) -> int:
    """
    Returns the main listener port number of the service set by command line
    `--service-port` option.
    If no port is specified in the command line option, keeps the original port
    specified in the static config.

    Override this fixture to change the way the main listener port number is
    detected by the testsuite.

    @ingroup userver_testsuite_fixtures
    """
    return pytestconfig.option.service_port or _get_port(
        _original_service_config,
        'listener',
        service_port,
        '--service-port',
    )


@pytest.fixture(scope='session')
def monitor_port(pytestconfig, _original_service_config) -> int:
    """
    Returns the monitor listener port number of the service set by command line
    `--monitor-port` option.
    If no port is specified in the command line option, keeps the original port
    specified in the static config.

    Override this fixture to change the way the monitor listener port number
    is detected by testsuite.

    @ingroup userver_testsuite_fixtures
    """
    return pytestconfig.option.monitor_port or _get_port(
        _original_service_config,
        'listener-monitor',
        monitor_port,
        '--service-port',
    )


def _get_port(
    original_service_config,
    listener_name,
    port_fixture,
    option_name,
) -> int:
    config_yaml = original_service_config.config_yaml
    config_vars = original_service_config.config_vars
    components = config_yaml['components_manager']['components']
    listener = components.get('server', {}).get(listener_name, {})
    if not listener:
        return -1
    port = listener.get('port', None)
    if isinstance(port, str) and port.startswith('$'):
        port = config_vars.get(port[1:], None) or listener.get(
            'port#fallback',
            None,
        )
    assert port, (
        f'Please specify '
        f'components_manager.components.server.{listener_name}.port '
        f'in the static config, or pass {option_name} pytest option, '
        f'or override the {port_fixture.__name__} fixture'
    )
    return _choose_free_port(port)


# Beware: global variable
allocated_ports = set()


def _choose_free_port(first_port):
    def _try_port(port):
        global allocated_ports
        if port in allocated_ports:
            return None
        try:
            server.bind(('0.0.0.0', port))
            allocated_ports.add(port)
            return port
        except BaseException:
            return None

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server:
        for port in range(first_port, first_port + 100):
            if port := _try_port(port):
                return port

        for _attempt in random.sample(range(1024, 65535), k=100):
            if port := _try_port(port):
                return port

        assert False, 'Failed to pick a free TCP port'


@pytest.fixture(scope='session')
def userver_base_prepare_service_config():
    def patch_config(config, config_vars):
        components = config['components_manager']['components']
        if 'congestion-control' in components:
            if components['congestion-control'] is None:
                components['congestion-control'] = {}

            components['congestion-control']['fake-mode'] = True

    return patch_config
