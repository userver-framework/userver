import typing
import pytest
import redis as redisdb

from testsuite.databases.redis import service
from testsuite.environment.service import ScriptService
from testsuite.databases.redis import genredis
from testsuite.environment import utils

import pathlib
import logging

DEFAULT_STANDALONE_PORT = 7000
SERVICE_SCRIPT_PATH = pathlib.Path(__file__).parent.joinpath(
    'service-redis-standalone'
)

class StandaloneServiceSettings(typing.NamedTuple):
    host: str
    port: int

def get_service_settings():
    return StandaloneServiceSettings(
        host=service._get_hostname(),
        port=utils.getenv_int(
            key='TESTSUITE_REDIS_STANDALONE_PORT',
            default=DEFAULT_STANDALONE_PORT,
        )
    )

def create_redis_standalone(
    service_name,
    working_dir,
    settings: typing.Optional[StandaloneServiceSettings] = None,
    env=None,
):
    if settings is None:
        settings = get_service_settings()
    configs_dir = pathlib.Path(working_dir).joinpath('configs')
    input_file = genredis._redis_config_directory() / genredis.MASTER_TPL_FILENAME
    output_file = configs_dir.joinpath(f"{service_name}.conf")
    
    logging.debug(f"Config file for redis standalone is '{output_file}'")

    def prestart_hook():
        configs_dir.mkdir(parents=True, exist_ok=True)
        protected_mode_no = ''
        if genredis.redis_version() >= (3, 2, 0):
            protected_mode_no = 'protected-mode no'

        genredis._generate_redis_config(
            input_file, output_file, protected_mode_no, settings.host, settings.port
        )

    return ScriptService(
        service_name=service_name,
        script_path=str(SERVICE_SCRIPT_PATH),
        working_dir=working_dir,
        environment={
            'REDIS_CONFIG_FILE': output_file,
            **(env or {}),
        },
        check_host=settings.host,
        check_ports=[settings.port],
        prestart_hook=prestart_hook,
    )

def pytest_service_register(register_service):
    register_service('redis-standalone', create_redis_standalone)

@pytest.fixture(scope='session')
def redis_standalone_settings():
    return get_service_settings()

@pytest.fixture(scope='session')
async def redis_standalone_service(
    pytestconfig,
    ensure_service_started,
    redis_standalone_settings
):
    if not pytestconfig.option.no_redis:
        ensure_service_started('redis-standalone', settings=redis_standalone_settings)

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
