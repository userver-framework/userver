import pytest

from testsuite.daemons import spawn


async def test_failed_start_doesnt_hang(
    ensure_daemon_started,
    service_daemon_scope,
    service_baseurl,
    monitor_baseurl,
):
    with pytest.raises((spawn.HealthCheckError, spawn.ExitCodeError)):
        await ensure_daemon_started(service_daemon_scope)
