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
        is_testsuite_support_enabled,
        _service_client_base,
        _service_client_testsuite,
):
    await ensure_daemon_started(service_daemon)
    if is_testsuite_support_enabled:
        return _service_client_testsuite
    return _service_client_base


@pytest.fixture
def monitor_client(
        service_client, monitor_baseurl, service_client_options, mockserver,
):
    aiohttp_client = client.AiohttpClientMonitor(
        monitor_baseurl,
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
):
    aiohttp_client = client.AiohttpClient(
        service_baseurl,
        log_capture_fixture=userver_log_capture,
        mocked_time=mocked_time,
        **service_client_options,
    )
    return client.Client(aiohttp_client)


@pytest.fixture(scope='session')
def service_baseurl(pytestconfig):
    return f'http://localhost:{pytestconfig.option.service_port}/'


@pytest.fixture(scope='session')
def monitor_baseurl(pytestconfig):
    return f'http://localhost:{pytestconfig.option.monitor_port}/'


@pytest.fixture(scope='session')
def is_testsuite_support_enabled(service_config_yaml) -> bool:
    components = service_config_yaml['components_manager']['components']
    return 'tests-control' in components
