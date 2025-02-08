"""
Service main and monitor clients.
"""

# pylint: disable=redefined-outer-name
import typing

import aiohttp.client_exceptions
import pytest
import websockets

from testsuite.daemons import service_client as base_service_client
from testsuite.utils import compat

from pytest_userver import client


@pytest.fixture
async def service_client(
    service_daemon_instance,
    service_baseurl,
    service_client_options,
    userver_service_client_options,
    userver_client_cleanup,
    _testsuite_client_config: client.TestsuiteClientConfig,
) -> client.Client:
    """
    Main fixture that provides access to userver based service.

    @snippet samples/testsuite-support/tests/test_ping.py service_client
    @anchor service_client
    @ingroup userver_testsuite_fixtures
    """
    if not _testsuite_client_config.testsuite_action_path:
        yield _ClientDiagnose(service_baseurl, **service_client_options)
        return

    aiohttp_client = client.AiohttpClient(
        service_baseurl,
        **userver_service_client_options,
    )
    userver_client = client.Client(aiohttp_client)
    async with userver_client_cleanup(userver_client):
        yield userver_client


@pytest.fixture
def userver_client_cleanup(
    request,
    _userver_logging_plugin,
    _dynamic_config_defaults_storage,
    _check_config_marks,
    dynamic_config,
) -> typing.Callable[[client.Client], typing.AsyncGenerator]:
    """
    Contains the pre-test and post-test setup that depends
    on @ref service_client.

    Feel free to override, but in that case make sure to call the original
    `userver_client_cleanup` fixture instance.

    @ingroup userver_testsuite_fixtures
    """
    marker = request.node.get_closest_marker('suspend_periodic_tasks')
    if marker:
        tasks_to_suspend = marker.args
    else:
        tasks_to_suspend = ()

    @compat.asynccontextmanager
    async def cleanup_manager(client: client.Client):
        @_userver_logging_plugin.register_flusher
        async def do_flush():
            try:
                await client.log_flush()
            except aiohttp.client_exceptions.ClientError:
                pass
            except RuntimeError:
                # TODO: find a better way to handle closed aiohttp session
                pass

        # Service is already started we don't want startup logs to be shown
        _userver_logging_plugin.update_position()

        await _dynamic_config_defaults_storage.update(client, dynamic_config)
        _check_config_marks()

        await client.suspend_periodic_tasks(tasks_to_suspend)
        try:
            yield
        finally:
            await client.resume_all_periodic_tasks()

    return cleanup_manager


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
                service_client,
                'update_server_state',
                None,
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
    service_client,  # For daemon setup and userver_client_cleanup
    userver_monitor_client_options,
    monitor_baseurl,
) -> client.ClientMonitor:
    """
    Main fixture that provides access to userver monitor listener.

    @snippet samples/testsuite-support/tests/test_metrics.py metrics labels
    @ingroup userver_testsuite_fixtures
    """
    aiohttp_client = client.AiohttpClientMonitor(
        monitor_baseurl,
        **userver_monitor_client_options,
    )
    return client.ClientMonitor(aiohttp_client)


# @cond


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


@pytest.fixture
def userver_service_client_options(
    service_client_options,
    service_client_default_headers,
    mockserver,
    _testsuite_client_config: client.TestsuiteClientConfig,
    testpoint,
    testpoint_control,
    service_periodic_tasks_state,
    userver_log_capture,
    mocked_time,
    cache_invalidation_state,
    userver_cache_control,
    asyncexc_check,
    userver_allow_all_caches_invalidation,
):
    return dict(
        **service_client_options,
        headers={  # TODO merge into service_client_options
            **service_client_default_headers,
            mockserver.trace_id_header: mockserver.trace_id,  # TODO merge into service_client_default_headers
        },
        config=_testsuite_client_config,
        testpoint=testpoint,
        testpoint_control=testpoint_control,
        periodic_tasks_state=service_periodic_tasks_state,
        log_capture_fixture=userver_log_capture,
        mocked_time=mocked_time,
        cache_invalidation_state=cache_invalidation_state,
        cache_control=userver_cache_control,
        asyncexc_check=asyncexc_check,
        allow_all_caches_invalidation=userver_allow_all_caches_invalidation,
    )


@pytest.fixture
def userver_monitor_client_options(
    service_client_options,
    service_client_default_headers,
    mockserver,
    _testsuite_client_config: client.TestsuiteClientConfig,
):
    return dict(
        **service_client_options,
        headers={  # TODO merge into service_client_options
            **service_client_default_headers,
            mockserver.trace_id_header: mockserver.trace_id,  # TODO merge into service_client_default_headers
        },
        config=_testsuite_client_config,
    )


# @endcond


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
    pytestconfig,
    service_config,
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
