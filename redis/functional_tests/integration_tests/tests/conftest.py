import json
import os
import subprocess
import asyncio
import pytest
from testsuite.utils import callinfo
from testsuite.daemons import service_daemon

pytest_plugins = ['pytest_userver.plugins.redis']

os.environ['TESTSUITE_REDIS_HOSTNAME'] = 'localhost'

@pytest.fixture(scope='session')
async def redis_standalone_port():
    return 7000

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
def logger_plugin(pytestconfig):
    return pytestconfig.pluginmanager.getplugin('testsuite_logger')

@pytest.fixture(scope='session')
async def redis_standalone_run_command(redis_standalone_port):
    return [
                'redis-server',
                '/source/redis/functional_tests/integration_tests/tests/redis_standalone.conf',
                '--port',
                f'{redis_standalone_port}'
            ]

@pytest.fixture(scope='session')
async def redis_standalone(
    health_check,
    logger_plugin,
    redis_standalone_run_command
):

    async with service_daemon.start(
            args=redis_standalone_run_command,
            logger_plugin=logger_plugin,
            health_check=health_check,
            subprocess_options={'stderr': subprocess.PIPE, 'bufsize': 0}
    ) as scope:
        await asyncio.sleep(1.0) # wait_service_started(args=redis_standalone_run_command, health_check=health_check)
        yield scope

@pytest.fixture(scope='session')
def service_env(redis_sentinels, redis_cluster_nodes, redis_cluster_replicas, redis_standalone, redis_standalone_port):
    cluster_shards = [
        {'name': f'shard{idx}'}
        for idx in range(
            len(redis_cluster_nodes) // (redis_cluster_replicas + 1),
        )
    ]

    secdist_config = {
        'redis_settings': {
            'redis-cluster': {
                'password': '',
                'sentinels': redis_cluster_nodes,
                'shards': cluster_shards,
            },
            'redis-sentinel': {
                'password': '',
                'sentinels': redis_sentinels,
                'shards': [{'name': 'test_master1'}],
            },
            'redis-standalone': {
                'password': '',
                'sentinels': [
                    {
                        "host": "localhost",
                        "port": redis_standalone_port
                    },
                ],
                'shards': [{'name': 'test_master1'}],
            },
        },
    }

    return {'SECDIST_CONFIG': json.dumps(secdist_config)}
