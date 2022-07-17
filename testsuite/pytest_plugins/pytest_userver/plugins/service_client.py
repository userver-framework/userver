import pytest
from testsuite.daemons import service_client

from pytest_userver import client


@pytest.fixture
def client_deps():
    """
    Service client dependencies hook. Feel free to override, e.g.:

    @pytest.fixture
    def client_deps(dep1, dep2, ...):
        pass
    """


@pytest.fixture(name='service_client')
async def _service_client(
        ensure_daemon_started,
        mockserver,
        service_daemon,
        client_deps,
        _testsuite_client_config: client.TestsuiteClientConfig,
        _service_client_base,
        _service_client_testsuite,
):
    await ensure_daemon_started(service_daemon)
    if _testsuite_client_config.testsuite_action_path:
        return _service_client_testsuite
    return _service_client_base


@pytest.fixture
def monitor_client(
        service_client,
        service_client_options,
        mockserver,
        monitor_baseurl: str,
        _testsuite_client_config: client.TestsuiteClientConfig,
):
    aiohttp_client = client.AiohttpClientMonitor(
        monitor_baseurl,
        config=_testsuite_client_config,
        headers={'x-yatraceid': mockserver.trace_id},
        **service_client_options,
    )
    return client.ClientMonitor(aiohttp_client)


@pytest.fixture
async def _service_client_base(service_baseurl, service_client_options):
    return service_client.Client(service_baseurl, **service_client_options)


@pytest.fixture
async def _service_client_testsuite(
        service_baseurl,
        service_client_options,
        mocked_time,
        userver_log_capture,
        testpoint,
        testpoint_control,
        _testsuite_client_config: client.TestsuiteClientConfig,
):
    aiohttp_client = client.AiohttpClient(
        service_baseurl,
        config=_testsuite_client_config,
        testpoint=testpoint,
        testpoint_control=testpoint_control,
        log_capture_fixture=userver_log_capture,
        mocked_time=mocked_time,
        **service_client_options,
    )
    return client.Client(aiohttp_client)


@pytest.fixture(scope='session')
def service_baseurl(service_port):
    return f'http://localhost:{service_port}/'


@pytest.fixture(scope='session')
def monitor_baseurl(monitor_port):
    return f'http://localhost:{monitor_port}/'


@pytest.fixture(scope='session')
def _testsuite_client_config(
        pytestconfig, service_config_yaml,
) -> client.TestsuiteClientConfig:
    components = service_config_yaml['components_manager']['components']

    def get_component_path(name, argname=None):
        if name in components:
            path = components[name]['path']
            path = path.rstrip('*')

            if argname and f'{{{argname}}}' not in path:
                raise RuntimeError(
                    f'Component {name} must provide path argument {argname}',
                )
            return path
        return None

    return client.TestsuiteClientConfig(
        server_monitor_path=get_component_path('handler-server-monitor'),
        testsuite_action_path=get_component_path('tests-control', 'action'),
    )
