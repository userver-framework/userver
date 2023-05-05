import typing

import pytest

import taxi.uservices.userver.scripts.uctl.uctl as uctl


@pytest.fixture(scope='session')
def run_uctl(service_config_path_temp):
    async def _uctl(cmdline: typing.List[str]) -> str:
        return await uctl.main(
            ['--config', str(service_config_path_temp)] + cmdline,
        )

    return _uctl


async def test_log_level_get(monitor_client, run_uctl):
    assert await run_uctl(['log-level', 'get']) == 'debug'
