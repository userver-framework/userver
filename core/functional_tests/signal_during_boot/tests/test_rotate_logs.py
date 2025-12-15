import os
import signal

import pytest_userver.utils.sync as sync

TIMEOUT = 60


async def wait_for_logfile(fname: str):
    async def check_ready():
        with open(fname) as ifile:
            for line in ifile.readlines():
                if 'Starting to catch signals' in line:
                    return
        raise sync.NotReady()

    await sync.wait_until(check_ready)


async def test_boot_logs(
    service_binary,
    service_config_path_temp,
    service_env,
    create_daemon_scope,
    ensure_daemon_started,
    _service_logfile_path,
):
    async def _checker(*, session, process) -> bool:
        return True

    async with create_daemon_scope(
        args=[
            str(service_binary),
            '--config',
            str(service_config_path_temp),
        ],
        health_check=_checker,
        env=service_env,
    ) as scope:
        daemon = await ensure_daemon_started(scope)

        await wait_for_logfile(_service_logfile_path)

        os.remove(_service_logfile_path)
        assert not os.path.exists(_service_logfile_path)

        daemon.process.send_signal(signal.SIGUSR1)

        async def check_ready():
            if not os.path.exists(_service_logfile_path):
                raise sync.NotReady()

        await sync.wait_until(check_ready)
