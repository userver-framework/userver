"""
Start the service in testsuite.
"""

# pylint: disable=redefined-outer-name
import logging
import pathlib
import time
import typing

import pytest

from testsuite.utils import url_util

from ..utils import net


logger_testsuite = logging.getLogger(__name__)


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


@pytest.fixture(scope='session')
def service_env():
    """
    Override this to pass extra environment variables to the service.

    @snippet samples/redis_service/tests/conftest.py service_env
    @ingroup userver_testsuite_fixtures
    """
    return None


@pytest.fixture(scope='session')
async def service_http_ping_url(
        service_config, service_baseurl,
) -> typing.Optional[str]:
    """
    Returns the service HTTP ping URL that is used by the testsuite to detect
    that the service is ready to work. Returns None if there's no such URL.

    By default attempts to find server::handlers::Ping component by
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
async def service_daemon(
        pytestconfig,
        create_daemon_scope,
        service_env,
        service_http_ping_url,
        service_config_path_temp,
        service_config,
        service_binary,
        service_non_http_health_checks,
        testsuite_logger,
):
    """
    Configures the health checking to use service_http_ping_url fixture value
    if it is not None; otherwise uses the service_non_http_health_checks info.
    Starts the service daemon.

    @ingroup userver_testsuite_fixtures
    """
    assert service_http_ping_url or service_non_http_health_checks.tcp, (
        '"service_http_ping_url" and "create_health_checker" fixtures '
        'returned None. Testsuite is unable to detect if the service is ready '
        'to accept requests.',
    )

    logger_testsuite.debug(
        'userver fixture "service_daemon" would check for "%s"',
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
            logger_testsuite.debug(
                'userver fixture "service_daemon" checking "%s", attempt %s',
                service_non_http_health_checks,
                LocalCounters.attempts,
            )

        return await net.check_availability(service_non_http_health_checks)

    health_check = _checker
    if service_http_ping_url:
        health_check = None

    async with create_daemon_scope(
            args=[
                str(service_binary),
                '--config',
                str(service_config_path_temp),
            ],
            ping_url=service_http_ping_url,
            health_check=health_check,
            env=service_env,
    ) as scope:
        yield scope
