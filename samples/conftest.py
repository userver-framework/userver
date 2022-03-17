import pathlib

import pytest
from testsuite.daemons import service_client
import yaml

pytest_plugins = [
    'testsuite.pytest_plugin',
    'testsuite.databases.mongo.pytest_plugin',
    'testsuite.databases.pgsql.pytest_plugin',
    'testsuite.databases.redis.pytest_plugin',
]


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
    group.addoption(
        '--test-monitor-port',
        help='Bind example monitor to this port (default is %(default)s)',
        default=8086,
        type=int,
    )


@pytest.fixture
def client_deps():
    pass


@pytest.fixture
async def test_service_client(
        ensure_daemon_started,
        service_client_options,
        mockserver,
        test_service_daemon,
        test_service_baseurl,
        client_deps,
):
    await ensure_daemon_started(test_service_daemon)
    return service_client.Client(
        test_service_baseurl, **service_client_options,
    )


@pytest.fixture
def test_monitor_client(
        test_service_client, test_monitor_baseurl, service_client_options,
):
    return service_client.Client(
        test_monitor_baseurl, **service_client_options,
    )


@pytest.fixture(scope='session')
def build_dir(pytestconfig) -> pathlib.Path:
    return pathlib.Path(pytestconfig.option.build_dir).resolve()


@pytest.fixture(scope='session')
def service_source_dir(pytestconfig):
    return pytestconfig.option.service_source_dir


@pytest.fixture(scope='session')
def test_service_baseurl(pytestconfig):
    return f'http://localhost:{pytestconfig.option.test_service_port}/'


@pytest.fixture(scope='session')
def test_monitor_baseurl(pytestconfig):
    return f'http://localhost:{pytestconfig.option.test_monitor_port}/'


@pytest.fixture(scope='session')
def test_service_env():
    return None


@pytest.fixture(scope='session')
async def test_service_daemon(
        pytestconfig,
        create_daemon_scope,
        test_service_baseurl,
        test_service_env,
        service_config_path,
        mockserver_info,
):
    async with create_daemon_scope(
            args=[
                str(pytestconfig.option.service_binary),
                '--config',
                str(service_config_path),
            ],
            check_url=test_service_baseurl + 'ping',
            env=test_service_env,
    ) as scope:
        yield scope


@pytest.fixture(scope='session')
def service_config_path(
        pytestconfig,
        tmp_path_factory,
        build_dir,
        mockserver_info,
        service_source_dir: pathlib.Path,
        patch_service_yaml,
) -> pathlib.Path:
    destination = tmp_path_factory.mktemp(pytestconfig.option.service_name)

    src_path = service_source_dir.joinpath('static_config.yaml')
    dst_path = destination / src_path.name

    with src_path.open('rt') as fp:
        config_yaml = yaml.safe_load(fp)

    if 'config_vars' in config_yaml:
        config_yaml['config_vars'] = str(
            service_source_dir.joinpath('config_vars.yaml'),
        )

    components = config_yaml['components_manager']['components']
    server = components['server']
    server['listener']['port'] = pytestconfig.option.test_service_port

    if 'listener-monitor' in server:
        server['listener-monitor'][
            'port'
        ] = pytestconfig.option.test_monitor_port

    if 'taxi-config-fallbacks' in components:
        components['taxi-config-fallbacks']['fallback-path'] = str(
            service_source_dir.joinpath('dynamic_config_fallback.json'),
        )

    if 'taxi-config-client-updater' in components:
        components['taxi-config-client-updater']['fallback-path'] = str(
            service_source_dir.joinpath('dynamic_config_fallback.json'),
        )

    if 'tests-control' in components:
        components['tests-control']['testpoint-url'] = mockserver_info.url(
            'testpoint',
        )

    if 'logging' in components:
        components['logging']['loggers'] = {
            'default': {
                'file_path': '@stderr',
                'level': 'debug',
                'overflow_behavior': 'discard',
            },
        }

    if 'secdist' in components:
        components['secdist']['config'] = str(
            service_source_dir.joinpath('secure_data.json'),
        )

    patch_service_yaml(config_yaml)

    dst_path.write_text(yaml.dump(config_yaml))
    return dst_path


@pytest.fixture(scope='session')
def patch_service_yaml():
    def do_nothing(config_yaml):
        pass

    return do_nothing
