import pathlib
import string

import pytest

from testsuite.daemons import service_client

pytest_plugins = ['testsuite.pytest_plugin']


def pytest_addoption(parser) -> None:
    group = parser.getgroup('userver')
    group.addoption(
        '--build-dir',
        type=pathlib.Path,
        help='Path to service build directory.',
        required=True,
    )
    group.addoption('--service-name', help='Service name', required=True)

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
        '--test-service-port',
        help='Bind example services to this port (default is %(default)s)',
        default=8080,
        type=int,
    )


@pytest.fixture
async def test_service_client(
        ensure_daemon_started,
        service_client_options,
        mockserver,
        test_service_daemon,
        test_service_baseurl,
):
    await ensure_daemon_started(test_service_daemon)
    return service_client.Client(
        test_service_baseurl, **service_client_options,
    )


@pytest.fixture(scope='session')
def build_dir(request) -> pathlib.Path:
    return pathlib.Path(request.config.getoption('--build-dir')).resolve()


@pytest.fixture(scope='session')
def test_service_baseurl(pytestconfig):
    return f'http://localhost:{pytestconfig.option.test_service_port}/'


@pytest.fixture(scope='session')
async def test_service_daemon(
        pytestconfig,
        tmp_path_factory,
        create_daemon_scope,
        build_dir,
        test_service_baseurl,
        mockserver_info,
):
    configs_path = pytestconfig.option.service_source_dir.joinpath('configs')
    temp_dir_name = tmp_path_factory.mktemp(pytestconfig.option.service_name)

    _copy_service_configs(
        service_name=pytestconfig.option.service_name,
        service_source_dir=pytestconfig.option.service_source_dir,
        destination=temp_dir_name,
        configs_path=configs_path,
        service_port=pytestconfig.option.test_service_port,
        mockserver_info=mockserver_info,
    )

    async with create_daemon_scope(
            args=[
                str(pytestconfig.option.service_binary),
                '--config',
                str(temp_dir_name.joinpath('static_config.yaml')),
            ],
            check_url=test_service_baseurl + 'ping',
    ) as scope:
        yield scope


def _copy_service_configs(
        *,
        service_name: str,
        service_source_dir: pathlib.Path,
        destination: pathlib.Path,
        configs_path: pathlib.Path,
        service_port: int,
        mockserver_info,
) -> None:
    for source_path in configs_path.iterdir():
        if not source_path.is_file():
            continue

        template = string.Template(source_path.read_text())
        data = template.substitute(
            service_name=service_name,
            service_dir=service_source_dir,
            port=service_port,
            testsuite=True,
            mockserver=mockserver_info.base_url,
        )

        (destination / source_path.name).write_text(data)
