"""
Service main and monitor clients.
"""

import logging

import pytest
import websockets

from testsuite.daemons import service_client as base_service_client
from testsuite.utils import compat

from pytest_userver import client


logger = logging.getLogger(__name__)


@pytest.fixture(name='extra_client_deps')
def _extra_client_deps() -> None:
    """
    Service client dependencies hook. Feel free to override, e.g.:

    @code
    @pytest.fixture
    def extra_client_deps(some_fixtures_to_wait_before_service_start):
        pass
    @endcode
    @ingroup userver_testsuite_fixtures
    """


@pytest.fixture(name='auto_client_deps')
def _auto_client_deps(request) -> None:
    """
    Service client dependencies hook that knows about pgsql, mongodb,
    clickhouse, rabbitmq, redis_store, ydb, and mysql dependencies.
    To add some other dependencies prefer overriding the
    extra_client_deps() fixture.

    @ingroup userver_testsuite_fixtures
    """
    known_deps = {
        'pgsql',
        'mongodb',
        'clickhouse',
        'rabbitmq',
        'redis_cluster_store',
        'redis_store',
        'mysql',
        'ydb',
    }

    try:
        fixture_lookup_error = pytest.FixtureLookupError
    except AttributeError:
        # support for an older version of the pytest
        import _pytest.fixtures
        fixture_lookup_error = _pytest.fixtures.FixtureLookupError

    resolved_deps = []
    for dep in known_deps:
        try:
            request.getfixturevalue(dep)
            resolved_deps.append(dep)
        except fixture_lookup_error:
            pass

    logger.debug(
        'userver fixture "auto_client_deps" resolved dependencies %s',
        resolved_deps,
    )


@pytest.fixture(name='service_client')
async def _service_client(
        ensure_daemon_started,
        service_daemon,
        dynamic_config,
        mock_configs_service,
        cleanup_userver_dumps,
        extra_client_deps,
        auto_client_deps,
        _config_service_defaults_updated,
        _testsuite_client_config: client.TestsuiteClientConfig,
        _service_client_base,
        _service_client_testsuite,
) -> client.Client:
    """
    Main fixture that provides access to userver based service.

    @snippet samples/testsuite-support/tests/test_ping.py service_client
    @anchor service_client
    @ingroup userver_testsuite_fixtures
    """
    # The service is lazily started here (not at the 'session' scope)
    # to allow '*_client_deps' to be active during service start
    await ensure_daemon_started(service_daemon)

    if not _testsuite_client_config.testsuite_action_path:
        return _service_client_base

    await _config_service_defaults_updated.update(
        _service_client_testsuite, dynamic_config,
    )
    return _service_client_testsuite


@pytest.fixture
async def websocket_client(service_client, service_port):
    """
    Fixture that provides access to userver based websocket service.

    @anchor websocket_client
    @ingroup userver_testsuite_fixtures
    """

    class Client:
        @compat.asynccontextmanager
        async def get(self, path):
            update_server_state = getattr(
                service_client, 'update_server_state', None,
            )
            if update_server_state:
                await update_server_state()
            ws_context = websockets.connect(
                f'ws://localhost:{service_port}/{path}',
            )
            async with ws_context as socket:
                yield socket

    return Client()


@pytest.fixture
def monitor_client(
        service_client,
        service_client_options,
        mockserver,
        monitor_baseurl: str,
        _testsuite_client_config: client.TestsuiteClientConfig,
) -> client.ClientMonitor:
    """
    Main fixture that provides access to userver monitor listener.

    @snippet samples/testsuite-support/tests/test_metrics.py metrics labels
    @ingroup userver_testsuite_fixtures
    """
    aiohttp_client = client.AiohttpClientMonitor(
        monitor_baseurl,
        config=_testsuite_client_config,
        headers={'x-yatraceid': mockserver.trace_id},
        **service_client_options,
    )
    return client.ClientMonitor(aiohttp_client)


@pytest.fixture(name='_service_client_base')
async def _service_client_base_fixture(
        service_baseurl, service_client_options,
):
    class _ClientDiagnose(base_service_client.Client):
        def __getattr__(self, name: str) -> None:
            raise AttributeError(
                f'"Client" object has no attribute "{name}". '
                'Note that "service_client" fixture returned the basic '
                '"testsuite.daemons.service_client.Client" client rather than '
                'a "pytest_userver.client.Client" client with userver '
                'extensions. That happened because the service '
                'static configuration file contains no "tests-control" '
                'component with "action" field.',
            )

    return _ClientDiagnose(service_baseurl, **service_client_options)


@pytest.fixture(name='_service_client_testsuite')
def _service_client_testsuite_fixture(
        service_baseurl,
        service_client_options,
        mocked_time,
        userver_log_capture,
        testpoint,
        testpoint_control,
        cache_invalidation_state,
        _testsuite_client_config: client.TestsuiteClientConfig,
) -> client.Client:
    aiohttp_client = client.AiohttpClient(
        service_baseurl,
        config=_testsuite_client_config,
        testpoint=testpoint,
        testpoint_control=testpoint_control,
        log_capture_fixture=userver_log_capture,
        mocked_time=mocked_time,
        cache_invalidation_state=cache_invalidation_state,
        **service_client_options,
    )
    return client.Client(aiohttp_client)


@pytest.fixture(name='service_baseurl', scope='session')
def _service_baseurl(service_port) -> str:
    """
    Returns the main listener URL of the service.

    Override this fixture to change the main listener URL that the testsuite
    uses for tests.

    @ingroup userver_testsuite_fixtures
    """
    return f'http://localhost:{service_port}/'


@pytest.fixture(name='monitor_baseurl', scope='session')
def _monitor_baseurl(monitor_port) -> str:
    """
    Returns the main monitor URL of the service.

    Override this fixture to change the main monitor URL that the testsuite
    uses for tests.

    @ingroup userver_testsuite_fixtures
    """
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
