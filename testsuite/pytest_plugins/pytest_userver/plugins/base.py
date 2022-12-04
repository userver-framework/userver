"""
Configure the service in testsuite.
"""

import pathlib

import pytest

from pytest_userver.utils import net as net_utils


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
        help='Bind example services to this port (default is %(default)s)',
        default=8080,
        type=int,
    )
    group.addoption(
        '--monitor-port',
        help='Bind example monitor to this port (default is %(default)s)',
        default=8086,
        type=int,
    )


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
def service_port(pytestconfig) -> int:
    """
    Returns the main listener port number of the service set by command line
    `--service-port` option.

    Override this fixture to change the way the main listener port number is
    detected by the testsuite.

    @ingroup userver_testsuite_fixtures
    """
    return pytestconfig.option.service_port


@pytest.fixture(scope='session')
def monitor_port(pytestconfig) -> int:
    """
    Returns the monitor listener port number of the service set by command line
    `--monitor-port` option.

    Override this fixture to change the way the monitor listener port number
    is detected by testsuite.

    @ingroup userver_testsuite_fixtures
    """
    return pytestconfig.option.monitor_port


@pytest.fixture(scope='session')
def create_port_health_checker():
    """
    Returns health checker factory function with sinature
    'def create_health_checker(*, hostname: str, port: int)' that should return
    'async def checker(*, session, process) -> bool' function for health
    checking of the service.

    Override this fixture to change the way testsuite detects the tested
    service being alive.

    @ingroup userver_testsuite_fixtures
    """

    def create_health_checker(*, hostname: str, port: int):
        async def checker(*, session, process):
            return await net_utils.check_port_availability(hostname, port)

        return checker

    return create_health_checker
