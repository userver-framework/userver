"""
Service main and monitor clients.
"""

# pylint: disable=redefined-outer-name
import logging
import typing

import aiohttp.client_exceptions
import pytest
import websockets

from testsuite.daemons import service_client as base_service_client
from testsuite.daemons.pytest_plugin import DaemonInstance
from testsuite.utils import compat

from pytest_userver import client


logger = logging.getLogger(__name__)


@pytest.fixture
def extra_client_deps() -> None:
    """
    Service client dependencies hook. Feel free to override, e.g.:

    @code
    @pytest.fixture
    def extra_client_deps(some_fixtures_to_wait_before_service_start):
        pass
    @endcode
    @ingroup userver_testsuite_fixtures
    """


@pytest.fixture
def auto_client_deps(request) -> None:
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


@pytest.fixture
async def service_client(
        ensure_daemon_started,
        service_daemon,
        dynamic_config,
        mock_configs_service,
        cleanup_userver_dumps,
        userver_client_cleanup,
        _config_service_defaults_updated,
        _testsuite_client_config: client.TestsuiteClientConfig,
        _service_client_base,
        _service_client_testsuite,
        # User defined client deps must be last in order to use
        # fixtures defined above.
        extra_client_deps,
        auto_client_deps,
) -> client.Client:
    """
    Main fixture that provides access to userver based service.

    @snippet samples/testsuite-support/tests/test_ping.py service_client
    @anchor service_client
    @ingroup userver_testsuite_fixtures
    """
    # The service is lazily started here (not at the 'session' scope)
    # to allow '*_client_deps' to be active during service start
    daemon = await ensure_daemon_started(service_daemon)

    if not _testsuite_client_config.testsuite_action_path:
        yield _service_client_base
    else:
        service_client = _service_client_testsuite(daemon)
        await _config_service_defaults_updated.update(
            service_client, dynamic_config,
        )

        async with userver_client_cleanup(service_client):
            yield service_client


@pytest.fixture
def userver_client_cleanup(request, userver_flush_logs):
    marker = request.node.get_closest_marker('suspend_periodic_tasks')
    if marker:
        tasks_to_suspend = marker.args
    else:
        tasks_to_suspend = ()

    @compat.asynccontextmanager
    async def cleanup_manager(client: client.AiohttpClient):
        async with userver_flush_logs(client):
            await client.suspend_periodic_tasks(tasks_to_suspend)
            try:
                yield client
            finally:
                await client.resume_all_periodic_tasks()

    return cleanup_manager


@pytest.fixture
def userver_flush_logs(request):
    """Flush logs in case of failure."""

    @compat.asynccontextmanager
    async def flush_logs(service_client: client.AiohttpClient):
        async def do_flush():
            try:
                await service_client.log_flush()
            except aiohttp.client_exceptions.ClientResponseError:
                pass

        failed = False
        try:
            yield
        except Exception:
            failed = True
            raise
        finally:
            item = request.node
            if failed or item.utestsuite_report.failed:
                await do_flush()

    return flush_logs


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


@pytest.fixture
async def _service_client_base(service_baseurl, service_client_options):
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


@pytest.fixture
def _service_client_testsuite(
        service_baseurl,
        service_client_options,
        mocked_time,
        userver_cache_control,
        userver_log_capture,
        testpoint,
        testpoint_control,
        cache_invalidation_state,
        service_periodic_tasks_state,
        _testsuite_client_config: client.TestsuiteClientConfig,
) -> typing.Callable[[DaemonInstance], client.Client]:
    def create_client(daemon):
        aiohttp_client = client.AiohttpClient(
            service_baseurl,
            config=_testsuite_client_config,
            testpoint=testpoint,
            testpoint_control=testpoint_control,
            periodic_tasks_state=service_periodic_tasks_state,
            log_capture_fixture=userver_log_capture,
            mocked_time=mocked_time,
            cache_invalidation_state=cache_invalidation_state,
            cache_control=userver_cache_control(daemon),
            **service_client_options,
        )
        return client.Client(aiohttp_client)

    return create_client


@pytest.fixture(scope='session')
def service_baseurl(service_port) -> str:
    """
    Returns the main listener URL of the service.

    Override this fixture to change the main listener URL that the testsuite
    uses for tests.

    @ingroup userver_testsuite_fixtures
    """
    return f'http://localhost:{service_port}/'


@pytest.fixture(scope='session')
def monitor_baseurl(monitor_port) -> str:
    """
    Returns the main monitor URL of the service.

    Override this fixture to change the main monitor URL that the testsuite
    uses for tests.

    @ingroup userver_testsuite_fixtures
    """
    return f'http://localhost:{monitor_port}/'


@pytest.fixture(scope='session')
def service_periodic_tasks_state() -> client.PeriodicTasksState:
    return client.PeriodicTasksState()


@pytest.fixture(scope='session')
def _testsuite_client_config(
        pytestconfig, service_config,
) -> client.TestsuiteClientConfig:
    components = service_config['components_manager']['components']

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
