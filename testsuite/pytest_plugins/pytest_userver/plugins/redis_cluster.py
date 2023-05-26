"""
Plugin that imports the required fixtures to start the Redis database in
cluster mode
"""

import json
import typing

import pytest
import redis

from testsuite.environment import utils


pytest_plugins = ['pytest_userver.plugins.core']


DEFAULT_CLUSTER_PORTS = (17380, 17381, 17382, 17383, 17384, 17385)
WAIT_TIMEOUT_MS = 1000


class BaseError(Exception):
    pass


class NotEnoughPorts(BaseError):
    pass


class ClusterSettings(typing.NamedTuple):
    host: str
    ports: typing.Tuple[int, ...]

    def validate(self):
        if len(self.ports) < len(DEFAULT_CLUSTER_PORTS):
            raise NotEnoughPorts(
                f'Need more than {len(DEFAULT_CLUSTER_PORTS)} cluster nodes!',
            )


@pytest.fixture(scope='session')
def _redis_service_settings():
    return ClusterSettings(
        host='localhost',  # TODO: how can we set host?
        ports=utils.getenv_ints(
            key='TESTSUITE_REDIS_CLUSTER_PORTS', default=DEFAULT_CLUSTER_PORTS,
        ),
    )


@pytest.fixture(scope='session')
def _cluster_redis_hosts(pytestconfig, _redis_service_settings):
    return [
        {'host': _redis_service_settings.host, 'port': port}
        for port in _redis_service_settings.ports
    ]


def _json_object_hook(dct):
    if '$json' in dct:
        return json.dumps(dct['$json'])
    return dct


@pytest.fixture
def redis_cluster_store(
        pytestconfig, request, load_json, _cluster_redis_hosts,
):
    """
    Fixture that provide client to redis databases in cluster mode.

    @ingroup userver_testsuite_fixtures
    """

    def _flush_all(redis_db):
        nodes = redis_db.get_primaries()
        redis_db.flushall(target_nodes=nodes)
        redis_db.wait(1, WAIT_TIMEOUT_MS, target_nodes=nodes)

    if pytestconfig.option.no_redis:
        yield
        return

    if len(_cluster_redis_hosts) == 0:
        yield
        return

    redis_commands = []

    for mark in request.node.iter_markers('redis_cluster_store'):
        store_file = mark.kwargs.get('file')
        if store_file is not None:
            redis_commands_from_file = load_json(
                '%s.json' % store_file, object_hook=_json_object_hook,
            )
            redis_commands.extend(redis_commands_from_file)

        if mark.args:
            redis_commands.extend(mark.args)

    redis_db = redis.RedisCluster(
        host=_cluster_redis_hosts[0]['host'],
        port=_cluster_redis_hosts[0]['port'],
    )

    for redis_command in redis_commands:
        func = getattr(redis_db, redis_command[0])
        func(*redis_command[1:])
    try:
        yield redis_db
    finally:
        _flush_all(redis_db)
