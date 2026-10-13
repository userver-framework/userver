import asyncio
import os
import signal

import pytest_userver.utils.sync as sync

TIMEOUT = 60


def stderr_handler(line: str):
    pass


async def test_boot_logs(
    service_binary,
    service_config_path_temp,
    service_env,
    create_daemon_scope,
    ensure_daemon_started,
    testpoint,
    _service_logfile_path,
):
    component_loading = asyncio.Event()

    @testpoint('slow-component-loading')
    def on_component_loading(_data):
        component_loading.set()

    async def _checker(*, session, process) -> bool:
        return component_loading.is_set()

    async with create_daemon_scope(
        args=[
            str(service_binary),
            '--config',
            str(service_config_path_temp),
        ],
        health_check=_checker,
        env=service_env,
        stderr_handler=stderr_handler,
    ) as scope:
        daemon = await ensure_daemon_started(scope)

        os.remove(_service_logfile_path)
        assert not os.path.exists(_service_logfile_path)

        daemon.process.send_signal(signal.SIGUSR1)

        async def check_ready():
            if not os.path.exists(_service_logfile_path):
                raise sync.NotReady()

        await sync.wait_until(check_ready)
