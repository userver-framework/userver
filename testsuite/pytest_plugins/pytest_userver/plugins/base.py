"""
Configure the service in testsuite.
"""

import pathlib

import pytest


class TestsuiteReport:
    def __init__(self):
        self.failed = False


def pytest_addoption(parser) -> None:
    group = parser.getgroup('userver')
    group.addoption(
        '--build-dir',
        type=pathlib.Path,
        help='Path to service build directory.',
    )

    group = parser.getgroup('Test service')
    group.addoption(
        '--service-binary', type=pathlib.Path, help='Path to service binary.',
    )
    group.addoption(
        '--service-port',
        help=(
            'Main HTTP port of the service '
            '(default: use the port from the static config)'
        ),
        default=None,
        type=int,
    )
    group.addoption(
        '--monitor-port',
        help=(
            'Monitor HTTP port of the service '
            '(default: use the port from the static config)'
        ),
        default=None,
        type=int,
    )
    group.addoption(
        '--service-source-dir',
        type=pathlib.Path,
        help='Path to service source directory.',
        default=pathlib.Path('.'),
    )


@pytest.hookimpl(hookwrapper=True, tryfirst=True)
def pytest_runtest_makereport(item, call):
    if not hasattr(item, 'utestsuite_report'):
        item.utestsuite_report = TestsuiteReport()
    outcome = yield
    rep = outcome.get_result()
    if rep.failed:
        item.utestsuite_report.failed = True
    return rep


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
        _original_service_config, 'listener', service_port, '--service-port',
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
        original_service_config, listener_name, port_fixture, option_name,
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
            'port#fallback', None,
        )
    assert port, (
        f'Please specify '
        f'components_manager.components.server.{listener_name}.port '
        f'in the static config, or pass {option_name} pytest option, '
        f'or override the {port_fixture.__name__} fixture'
    )
    return port
