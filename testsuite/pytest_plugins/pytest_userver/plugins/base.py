import pathlib

import pytest

from pytest_userver.utils import net as net_utils


def pytest_addoption(parser) -> None:
    group = parser.getgroup('userver')
    group.addoption(
        '--build-dir',
        type=pathlib.Path,
        help='Path to service build directory.',
        required=True,
    )

    group = parser.getgroup('Test service')
    group.addoption(
        '--service-binary',
        type=pathlib.Path,
        help='Path to service binary.',
        required=True,
    )
    group.addoption(
        '--service-source-dir',
        type=pathlib.Path,
        help='Path to service source directory.',
        required=True,
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
    return pathlib.Path(pytestconfig.option.build_dir).resolve()


@pytest.fixture(scope='session')
def service_source_dir(pytestconfig):
    return pytestconfig.option.service_source_dir


@pytest.fixture(scope='session')
def create_port_health_checker():
    def create_health_checker(*, hostname: str, port: int):
        async def checker(*, session, process):
            return await net_utils.check_port_availability(hostname, port)

        return checker

    return create_health_checker
