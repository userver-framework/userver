"""
Configure the service in testsuite.
"""

import pathlib

import pytest


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
    group.addoption(
        '--service-source-dir',
        type=pathlib.Path,
        help='Path to service source directory.',
        default=pathlib.Path('.'),
    )


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
