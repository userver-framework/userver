# pylint: disable=no-member
import asyncio
import dataclasses
import logging
import typing

# @cond


logger = logging.getLogger(__name__)


# @endcond


@dataclasses.dataclass(frozen=True)
class HostPort:
    """
    Class that holds a host and port.

    @ingroup userver_testsuite
    """

    host: str
    port: int


@dataclasses.dataclass(frozen=False)
class HealthChecks:
    """
    Class that holds all the info for health checks.

    @ingroup userver_testsuite
    """

    tcp: typing.List[HostPort] = dataclasses.field(default_factory=list)


async def _check_tcp_port_availability(tcp: HostPort) -> bool:
    try:
        _, writer = await asyncio.open_connection(tcp.host, tcp.port)
        writer.close()
        await writer.wait_closed()
    except (OSError, asyncio.TimeoutError) as exc:
        logger.debug('TCP %s:%s is not available: %s', tcp.host, tcp.port, exc)
        return False
    return True


async def check_availability(checks: HealthChecks) -> bool:
    """
    Checks availability for each provided entry.

    @ingroup userver_testsuite
    """
    assert checks.tcp
    done, pending = await asyncio.wait(
        [
            asyncio.Task(_check_tcp_port_availability(val))
            for val in checks.tcp
        ],
        timeout=25.0,
        return_when=asyncio.ALL_COMPLETED,
    )

    for task in pending:
        task.cancel()

    for task in pending:
        await task

    return not pending and all(task.result() for task in done)


def get_health_checks_info(service_config_yaml: dict) -> HealthChecks:
    """
    Returns a health checks info that for server.listener, grpc-server.port
    and server.listener-monitor.

    @see pytest_userver.plugins.base.service_non_http_health_checks()

    @ingroup userver_testsuite
    """
    checks = HealthChecks()

    components = service_config_yaml['components_manager']['components']
    server = components.get('server')
    if server:
        port = int(server.get('listener-monitor', {}).get('port', 0))
        if port:
            checks.tcp.append(HostPort('localhost', port))

        port = int(server.get('listener', {}).get('port', 0))
        if port:
            checks.tcp.append(HostPort('localhost', port))

    port = int(components.get('grpc-server', {}).get('port', 0))
    if port:
        checks.tcp.append(HostPort('localhost', port))

    return checks
