"""
Start the service in testsuite.
"""

# pylint: disable=redefined-outer-name
import logging
import pathlib
import re
import time
from typing import Any
from typing import Dict
from typing import List
from typing import Optional
from typing import Tuple

import pytest

from testsuite.daemons.pytest_plugin import DaemonInstance
from testsuite.utils import url_util

from pytest_userver.utils import net

logger = logging.getLogger(__name__)


def pytest_addoption(parser) -> None:
    group = parser.getgroup('userver')
    group.addoption(
        '--service-logs-file',
        type=pathlib.Path,
        help='Write service output to specified file',
    )
    group.addoption(
        '--service-logs-pretty',
        action='store_true',
        help='Enable pretty print and colorize service logs',
    )
    group.addoption(
        '--service-logs-pretty-verbose',
        dest='service_logs_pretty',
        action='store_const',
        const='verbose',
        help='Enable pretty print and colorize service logs in verbose mode',
    )
    group.addoption(
        '--service-logs-pretty-disable',
        action='store_false',
        dest='service_logs_pretty',
        help='Disable pretty print and colorize service logs',
    )
    group.addoption(
        '--service-live-logs-disable',
        action='store_true',
        help='Disable service live logs (enabled with -s)',
    )


@pytest.fixture(scope='session')
def service_env():
    """
    Override this to pass extra environment variables to the service.

    @snippet samples/redis_service/testsuite/conftest.py  service_env
    @ingroup userver_testsuite_fixtures
    """
    return None


@pytest.fixture(scope='session')
async def service_http_ping_url(
    service_config,
    service_baseurl,
) -> Optional[str]:
    """
    Returns the service HTTP ping URL that is used by the testsuite to detect
    that the service is ready to work. Returns None if there's no such URL.

    By default, attempts to find server::handlers::Ping component by
    "handler-ping" name in static config. Override this fixture to change the
    behavior.

    @ingroup userver_testsuite_fixtures
    """
    components = service_config['components_manager']['components']
    ping_handler = components.get('handler-ping')
    if ping_handler:
        return url_util.join(service_baseurl, ping_handler['path'])
    return None


@pytest.fixture(scope='session')
def service_non_http_health_checks(  # pylint: disable=invalid-name
    service_config,
) -> net.HealthChecks:
    """
    Returns a health checks info.

    By default, returns pytest_userver.utils.net.get_health_checks_info().

    Override this fixture to change the way testsuite detects the tested
    service being alive.

    @ingroup userver_testsuite_fixtures
    """

    return net.get_health_checks_info(service_config)


@pytest.fixture(scope='session')
async def service_daemon_scope(
    create_daemon_scope,
    daemon_scoped_mark,
    service_env,
    service_http_ping_url,
    service_config_path_temp,
    service_binary,
    service_non_http_health_checks,
):
    """
    Prepares the start of the service daemon.
    Configures the health checking to use service_http_ping_url fixture value
    if it is not None; otherwise uses the service_non_http_health_checks info.

    @see @ref service_daemon_instance
    @ingroup userver_testsuite_fixtures
    """
    assert service_http_ping_url or service_non_http_health_checks.tcp, (
        '"service_http_ping_url" and "create_health_checker" fixtures '
        'returned None. Testsuite is unable to detect if the service is ready '
        'to accept requests.',
    )

    logger.debug(
        'userver fixture "service_daemon_scope" would check for "%s"',
        service_non_http_health_checks,
    )

    class LocalCounters:
        last_log_time = 0.0
        attempts = 0

    async def _checker(*, session, process) -> bool:
        LocalCounters.attempts += 1
        new_log_time = time.monotonic()
        if new_log_time - LocalCounters.last_log_time > 1.0:
            LocalCounters.last_log_time = new_log_time
            logger.debug(
                'userver fixture "service_daemon_scope" checking "%s", attempt %s',
                service_non_http_health_checks,
                LocalCounters.attempts,
            )

        return await net.check_availability(service_non_http_health_checks)

    health_check = _checker
    if service_http_ping_url:
        health_check = None

    async with create_daemon_scope(
        args=[str(service_binary), '--config', str(service_config_path_temp)],
        ping_url=service_http_ping_url,
        health_check=health_check,
        env=service_env,
    ) as scope:
        yield scope


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
    clickhouse, rabbitmq, kafka, redis_store, ydb, and mysql dependencies.
    To add some other dependencies prefer overriding the
    @ref extra_client_deps fixture.

    @ingroup userver_testsuite_fixtures
    """
    known_deps = {
        'pgsql',
        'mongodb',
        'clickhouse',
        'rabbitmq',
        'kafka_producer',
        'kafka_consumer',
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
def builtin_client_deps(
    testpoint,
    cleanup_userver_dumps,
    userver_log_capture,
    dynamic_config,
    mock_configs_service,
):
    """
    Service client dependencies hook, like @ref extra_client_deps.

    Feel free to override globally in a more specific pytest plugin
    (one that comes after userver plugins),
    but make sure to depend on the original fixture:

    @code
    @pytest.fixture(name='builtin_client_deps')
    def _builtin_client_deps(builtin_client_deps, some_extra_fixtures):
        pass
    @endcode

    @ingroup userver_testsuite_fixtures
    """


@pytest.fixture
async def service_daemon_instance(
    ensure_daemon_started,
    service_daemon_scope,
    builtin_client_deps,
    auto_client_deps,
    # User defined client deps must be last in order to use
    # fixtures defined above.
    extra_client_deps,
) -> DaemonInstance:
    """
    Calls `ensure_daemon_started` on @ref userver_daemon_scope to actually
    start the service. Makes sure that all the dependencies are prepared
    before the service starts.

    @see @ref extra_client_deps
    @see @ref auto_client_deps
    @see @ref builtin_client_deps
    @ingroup userver_testsuite_fixtures
    """
    # TODO also run userver_client_cleanup here
    return await ensure_daemon_started(service_daemon_scope)


@pytest.fixture(scope='session')
def daemon_scoped_mark(request) -> Optional[Dict[str, Any]]:
    """
    Depend on this fixture directly or transitively to make your fixture a per-daemon fixture.

    Example:

    @code
    @pytest.fixture(scope='session')
    def users_cache_state(daemon_scoped_mark, ...):
        return UsersCacheState(users_list=[])
    @endcode

    For tests marked with `@pytest.mark.uservice_oneshot(...)`, the service will be restarted,
    and all the per-daemon fixtures will be recreated.

    This fixture returns kwargs passed to the `uservice_oneshot` mark (which may be an empty dict).
    For normal tests, this fixture returns `None`.

    @ingroup userver_testsuite_fixtures
    """
    # === How daemon-scoped fixtures work ===
    # pytest always keeps no more than 1 instance of each parametrized fixture.
    # When a parametrized fixture is requested, FixtureDef checks using __eq__ whether the current value
    # of fixture param equals the cached value. If they differ, then the fixture and all the dependent fixtures
    # are torn down (in reverse-dependency order), then the fixture is set up again.
    # TLDR: when the param changes, the whole tree of daemon-specific fixtures is torn down.
    return getattr(request, 'param', None)


# @cond


def pytest_configure(config):
    config.addinivalue_line(
        'markers',
        'uservice_oneshot: use a per-test service daemon instance',
    )


def pytest_generate_tests(metafunc: pytest.Metafunc) -> None:
    oneshot_marker = metafunc.definition.get_closest_marker('uservice_oneshot')
    if oneshot_marker:
        param = oneshot_marker.kwargs
        if param.get('shared', False):
            # Make sure that the param is not __eq__ to params of any other test.
            param = dict(param, function=metafunc.definition)
        metafunc.parametrize((daemon_scoped_mark.__name__,), [(param,)], indirect=True, ids=[_HIDDEN_PARAM])


_HIDDEN_PARAM = '__HIDDEN_PARAM__'
_HIDDEN_PATTERNS: Tuple[re.Pattern, ...] = (
    re.compile(rf'\[{_HIDDEN_PARAM}\d*]'),
    re.compile(rf'-{_HIDDEN_PARAM}\d*'),
    re.compile(rf'{_HIDDEN_PARAM}\d*-'),
)
_PARAM_PATTERN = re.compile(r'\[([^\]]*)]')


def pytest_collection_modifyitems(session: pytest.Session, config: pytest.Config, items: List[pytest.Item]):
    for item in items:
        # The fact that daemon_scoped_mark works through fixture parametrization is an implementation detail.
        # Hide the param from test output, making the feature transparent for the user.
        if _HIDDEN_PARAM not in item.name:
            continue
        for regex in _HIDDEN_PATTERNS:
            if regex.search(item.name):
                break
        else:
            assert False, f'Failed to mask test {item.name}'
        item.name = regex.sub('', item.name)
        item._nodeid = regex.sub('', item._nodeid)  # pylint: disable=protected-access
        item.keywords[item.name] = True
        if leftover_params := _PARAM_PATTERN.search(item.name):
            item.keywords[leftover_params.group(1)] = True


# @endcond
