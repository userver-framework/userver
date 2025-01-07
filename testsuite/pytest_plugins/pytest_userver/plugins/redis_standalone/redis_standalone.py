import os

import subprocess
import asyncio
import typing
import pytest
import redis as redisdb
from testsuite.utils import callinfo
from testsuite.daemons import service_daemon

from pathlib import Path

class StandaloneSettings(typing.NamedTuple):
    host: str
    port: int

@pytest.fixture(scope='session')
async def redis_standalone_config_path():
    return os.path.join(
        str(Path(__file__).parent),
        'redis_standalone.conf'
    )

@pytest.fixture(scope='session')
async def redis_standalone_port():
    return 7000

@pytest.fixture(scope='session')
def redis_standalone_settings(redis_standalone_port):
    return StandaloneSettings(
        host='localhost',
        port=redis_standalone_port
    )

@pytest.fixture(scope='session')
def health_check(redis_standalone_port):

    @callinfo.acallqueue
    async def health_check(*, process, session):
        print('Healt check called!')
        if not process:
            pytest.fail('process does not exist')
        if not process.pid:
            pytest.fail('process.pid is not set')
        return subprocess.run(["redis-cli", "-p", f"{redis_standalone_port}", "--raw", "incr", "ping"]).returncode == 0

    return health_check

@pytest.fixture(scope='session')
async def redis_standalone_run_command(redis_standalone_settings, redis_standalone_config_path):
    return [
                'redis-server',
                redis_standalone_config_path,
                '--port',
                f'{redis_standalone_settings.port}'
            ]

@pytest.fixture(scope='session')
async def redis_standalone_service(
    health_check,
    redis_standalone_run_command
):

    async with service_daemon.start(
            args=redis_standalone_run_command,
            health_check=health_check,
            subprocess_options={'stderr': subprocess.PIPE, 'bufsize': 0}
    ) as scope:
        await asyncio.sleep(1.0)
        yield scope

@pytest.fixture
def redis_standalone_store(
    pytestconfig,
    redis_standalone_service,
    redis_standalone_settings
):
    if pytestconfig.option.no_redis:
        yield
        return

    redis_db = redisdb.StrictRedis(
        host=redis_standalone_settings.host,
        port=redis_standalone_settings.port,
    )

    try:
        yield redis_db
    finally:
        redis_db.flushall()

@pytest.fixture(scope='session')
def redis_standalone(pytestconfig, redis_standalone_settings, redis_standalone_service):
    if pytestconfig.option.redis_host:
        # external Redis instance
        return [
            {
                'host': pytestconfig.option.redis_host,
                'port': (
                    pytestconfig.option.redis_sentinel_port
                    or redis_standalone_settings.port
                ),
            },
        ]
    return [
        {
            'host': redis_standalone_settings.host,
            'port': redis_standalone_settings.port,
        },
    ]
