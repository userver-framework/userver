import os
import subprocess
import sys
import typing

import pytest


try:
    import yatest.common.runtime
    UCTL_BIN = [
        yatest.common.runtime.build_path(
            'taxi/uservices/userver/scripts/uctl/uctl',
        ),
    ]
except ModuleNotFoundError:
    UCTL_BIN = [
        sys.executable,
        os.path.join(
            os.path.dirname(__file__),
            '..',
            '..',
            '..',
            '..',
            'scripts',
            'uctl',
            'uctl.py',
        ),
    ]


@pytest.fixture(scope='session')
def run_uctl(service_config_path_temp):
    async def _uctl(cmdline: typing.List[str]) -> str:
        return subprocess.check_output(
            UCTL_BIN + ['--config', str(service_config_path_temp)] + cmdline,
            encoding='utf-8',
        ).strip()

    return _uctl


async def test_log_level_get(monitor_client, run_uctl):
    assert await run_uctl(['log-level', 'get']) == 'debug'
