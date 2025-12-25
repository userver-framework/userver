import asyncio  # noqa: I001
import asyncio.subprocess
import logging
import os
import os.path
import platform
import shutil
import signal
import subprocess
import time

import pytest
import redis
import redis.asyncio as aredis
import yatest.common
import yatest.common.network

from library.python import resource
import pytest_userver.utils.sync as sync

TOTAL_WAIT_SECONDS = 180
CLUSTER_MINIMUM_NODES_COUNT = 6
REPLICA_COUNT = 1
REDIS_CLUSTER_HOST = '127.0.0.1'

logger = logging.getLogger(__name__)
port_manager = yatest.common.network.PortManager()


class RedisClusterTopologyError(Exception):
    pass


def _get_base_path() -> str:
    package_dir = 'taxi/uservices/userver/redis/functional_tests/pytest_redis_cluster_topology_plugin/package'
    return yatest.common.build_path(package_dir)


def _get_prefixed_path(*path_parts: str) -> str:
    return os.path.join(_get_base_path(), *path_parts)


async def _call_binary(binary_name: str, *args: str) -> None:
    logger.debug('Calling %s with %s', binary_name, args)
    proc = await asyncio.subprocess.create_subprocess_exec(
        _get_prefixed_path('bin', binary_name),
        *args,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        env={
            'DYLD_LIBRARY_PATH' if platform.system() == 'Darwin' else 'LD_LIBRARY_PATH': _get_prefixed_path('lib'),
        },
    )

    try:
        async with asyncio.timeout(15):
            outs, errs = await proc.communicate()
        logger.info(outs)
        logger.warning(errs)
        if proc.returncode:
            raise RuntimeError('Process exited with return code {proc.returncode}')
    except TimeoutError:
        logger.error('Timeout while waiting for {binary_name}, killing...')
        proc.kill()
        outs, errs = await proc.communicate()
        logger.info(outs)
        logger.warning(errs)
        raise


def _get_data_directory():
    return yatest.common.work_path('_redis_cluster_topology')


def _get_output_directory():
    return yatest.common.output_path('_redis_cluster_topology')


def _is_pid_running(pid: int) -> bool:
    try:
        os.kill(pid, 0)
        return True
    except ProcessLookupError:
        return False


class _RedisClusterNode:
    def __init__(self, host: str, port: int, cluster_port: int):
        config_name = 'cluster.conf'
        self.host = host
        self.port = port
        self.cluster_port = cluster_port
        self.data_directory = os.path.join(
            _get_data_directory(),
            f'redis_{host}:{port}',
        )
        self.output_directory = os.path.join(
            _get_output_directory(),
            f'redis_{host}:{port}',
        )
        os.makedirs(self.output_directory, exist_ok=True)
        self.pid_path = os.path.join(self.data_directory, 'redis.pid')
        self.config_path = os.path.join(self.output_directory, config_name)
        self.log_path = os.path.join(self.output_directory, 'redis.log')
        shutil.rmtree(self.data_directory, ignore_errors=True)
        os.makedirs(self.data_directory, exist_ok=True)
        self._write_config()
        self.started = False

        self.connection_pool = aredis.ConnectionPool.from_url(f'redis://{host}:{port}')

    def get_info(self, section=None):
        client = self.get_client()
        ret = client.info(section)
        return ret

    def get_client(self) -> redis.Redis:
        return redis.Redis(host=self.host, port=self.port)

    def get_async_client(self) -> aredis.Redis:
        return aredis.Redis(connection_pool=self.connection_pool)

    def get_primary_addresses(self) -> set[str]:
        try:
            client = self.get_client()
            cluster_slots = client.cluster('SLOTS')
        except (BaseException, redis.exceptions.ConnectionError):
            return set()
        ret = set()
        for interval in cluster_slots:
            master = interval[2]
            ret.add(f'{master[0].decode()}:{master[1]}')
        return ret

    def get_replica_addresses(self) -> set[str]:
        try:
            client = self.get_client()
            cluster_slots = client.cluster('SLOTS')
        except (BaseException, redis.exceptions.ConnectionError):
            return set()

        ret = set()
        for interval in cluster_slots:
            replicas = interval[3:]
            for replica in replicas:
                ret.add(f'{replica[0].decode()}:{replica[1]}')
        return ret

    def get_address(self):
        return f'{self.host}:{self.port}'

    async def start(self):
        if self.started:
            return
        args = [
            self.config_path,
            '--port',
            str(self.port),
            '--cluster-port',
            str(self.cluster_port),
            '--dir',
            self.data_directory,
            '--pidfile',
            'redis.pid',
            '--dbfilename',
            'redis.db',
            '--logfile',
            self.log_path,
        ]
        await _call_binary('redis-server', *args)
        await self.wait_ready()
        self.started = True

    def stop(self):
        pid = self._read_pid()
        if pid is None:
            logger.warning('%s is not running: no PID file', self.pid_path)
            return
        logger.info('Terminating %s: process PID %d', 'redis', pid)
        try:
            os.kill(pid, signal.SIGTERM)
        except ProcessLookupError:
            logger.info('Terminating %s: process PID %d seems already exited', 'redis', pid)
            return

        while _is_pid_running(pid):
            time.sleep(1)
        self.started = False

    def assert_running(self):
        pid = self._read_pid()
        assert pid is not None and _is_pid_running(pid), (
            f'Redis server process is dead, please look at the logfile {self.log_path}'
        )

    def _check_instance(self):
        with self.get_client() as client:
            try:
                client.ping()
            except redis.exceptions.ConnectionError as exc:
                logger.warning('Cannot get instance: %s', str(exc))
                self.assert_running()
                return False
        return True

    async def wait_ready(self):
        await sync.wait(
            check=self._check_instance,
            relax_msg='Redis cluster is not up yet, waiting',
            total_wait_seconds=TOTAL_WAIT_SECONDS,
        )
        logger.info(f'redis {self.get_address()} successfully pinged')

    def _write_config(self):
        resource_name = 'redis_cluster_config'
        content = resource.find(resource_name)
        assert content is not None, f'Could not find resource for {resource_name}'
        with open(self.config_path, 'w', encoding='utf-8') as config_file:
            config_file.write(content.decode())

    def _read_pid(self):
        try:
            with open(self.pid_path) as pid_file:
                return int(pid_file.read())
        except FileNotFoundError:
            return None
        except (OSError, ValueError):
            logger.exception('Could not read PID from %s', self.pid_path)
            return None


class RedisClusterTopology:
    def __init__(self, ports, cluster_ports):
        self.ports = ports
        self.all_ports = [(port, cport) for port, cport in zip(ports, cluster_ports, strict=True)]
        self.nodes = [
            _RedisClusterNode(REDIS_CLUSTER_HOST, port, cluster_port) for port, cluster_port in self.all_ports
        ]
        self.nodes_by_names = {node.get_address(): node for node in self.nodes}
        self.added_master = None
        self.added_replica = None
        self.slots_by_node = {}
        self.client = None

    async def start(self):
        await self.start_all_nodes()
        await self._create_cluster(self.all_ports)
        self.client = redis.RedisCluster(
            startup_nodes=[redis.cluster.ClusterNode('localhost', port) for port in self.ports],
        )

    def get_client(self):
        return self.client

    def get_masters(self) -> list[_RedisClusterNode]:
        ret: list[_RedisClusterNode] = []
        for node in self.nodes:
            addrs = node.get_primary_addresses()
            if not addrs:
                continue
            ret = []
            for addr in addrs:
                node = self.nodes_by_names.get(addr)
                if node:
                    ret.append(node)
            if ret:
                return ret
        return ret

    def get_replicas(self) -> list[_RedisClusterNode]:
        ret: list[_RedisClusterNode] = []
        for node in self.nodes:
            addrs = node.get_replica_addresses()
            if not addrs:
                continue
            ret = []
            for addr in addrs:
                node = self.nodes_by_names.get(addr)
                if node:
                    ret.append(node)
            if ret:
                return ret
        return ret

    async def add_shard(self):
        """
        Adds forth shard
        """
        # Validate state before proceed
        await self._wait_cluster_nodes_known(self.nodes, 6)
        await self._wait_cluster_nodes_ready(self.nodes, 6)

        # create 2 nodes
        new_master = _RedisClusterNode(
            REDIS_CLUSTER_HOST,
            port_manager.get_port(),
            port_manager.get_port(),
        )
        self.added_master = new_master
        await new_master.start()
        slot_set = set()
        self.slots_by_node[new_master.get_address()] = slot_set

        new_replica = _RedisClusterNode(
            REDIS_CLUSTER_HOST,
            port_manager.get_port(),
            port_manager.get_port(),
        )
        self.added_replica = new_replica
        await new_replica.start()
        self.slots_by_node[new_replica.get_address()] = slot_set

        # add new nodes to existing cluster
        old_masters = self.get_masters()
        await self._add_node_to_cluster(self.nodes[0], new_master)
        await self._wait_cluster_nodes_known(old_masters, len(self.nodes) + 1)

        await self._add_node_to_cluster(new_master, new_replica, replica=True)
        await self._wait_cluster_nodes_known(old_masters, len(self.nodes) + 2)

        nodes = old_masters + [new_master] + self.get_replicas() + [new_replica]
        # wait until nodes know each other
        await self._wait_cluster_nodes_known(nodes, len(nodes))

        # move part of hash slots from each of old master to new master
        slot_count = 16384 // 3 // 4
        for master in old_masters:
            await self._move_hash_slots(master, new_master, slot_count)

        # wait until all nodes know new cluster configuration
        await self._wait_cluster_nodes_ready(nodes, len(nodes))

    async def remove_shard(self):
        """
        Removes forth shard added by add_shard method
        """
        if self.added_master is None and self.added_replica is None:
            # already in desired state
            return

        # Validate state before proceed
        await self._wait_cluster_nodes_known(self.nodes, 8)
        await self._wait_cluster_nodes_ready(self.nodes, 8)
        new_master = self.added_master
        new_replica = self.added_replica
        original_masters = self.get_masters()
        original_replicas = self.get_replicas()

        slot_count = 16384 // 3 // 4
        for master in original_masters:
            await self._move_hash_slots(new_master, master, slot_count)
        new_master_id = new_master.get_client().cluster('myid').decode()
        new_replica_id = new_replica.get_client().cluster('myid').decode()
        time0 = time.time()
        for node in original_masters:
            client = node.get_client()
            client.cluster('forget', new_master_id)
            client.cluster('forget', new_replica_id)
        for node in original_replicas:
            client = node.get_client()
            client.cluster('forget', new_master_id)
            client.cluster('forget', new_replica_id)
        time1 = time.time()
        # Try to debug test flaps (TAXICOMMON-7684) with removing shard.
        # Maybe we somehow remove nodes so long that ban-list period of first
        # notified node elapses and it is getting removed node from gossip
        # again
        if time1 - time0 >= 60.0:
            raise RuntimeError(
                'Failed to notify all cluster nodes about deleted nodes within 1 minute',
            )

        await self._wait_cluster_nodes_ready(original_masters, 6)
        await self._wait_cluster_nodes_ready(original_replicas, 6)
        await self._wait_cluster_nodes_known(original_masters, 6)
        await self._wait_cluster_nodes_known(original_replicas, 6)

        new_master.stop()
        new_replica.stop()
        self.added_master = None
        self.added_replica = None

        # wait until just removed node will be removed from ban-list to allow
        # further addition of this node
        await asyncio.sleep(60)

    async def start_all_nodes(self):
        await asyncio.gather(*[node.start() for node in self.nodes])

        if self.added_master:
            await self.added_master.start()
        if self.added_replica:
            await self.added_replica.start()

    def cleanup(self):
        for node in self.nodes:
            node.stop()
        if self.added_master:
            self.added_master.stop()
        if self.added_replica:
            self.added_replica.stop()

    async def _move_slot(
        self,
        from_node: _RedisClusterNode,
        to_node: _RedisClusterNode,
        slot,
    ):
        from_client = from_node.get_async_client()
        to_client = to_node.get_async_client()
        try:
            from_id = (await from_client.cluster('myid')).decode()
            to_id = (await to_client.cluster('myid')).decode()
            await to_client.cluster('SETSLOT', slot, 'IMPORTING', from_id)
            await from_client.cluster('SETSLOT', slot, 'MIGRATING', to_id)

            batch = 1000
            timeout = 10000  # it flaps with 1000: 'IOERR error or timeout reading to target instance'
            db = 0
            while True:
                keys = await from_client.cluster('GETKEYSINSLOT', slot, batch)
                if not keys:
                    break
                await from_client.migrate(
                    REDIS_CLUSTER_HOST,
                    to_node.port,
                    keys,
                    db,
                    timeout,
                )
            await to_client.cluster('SETSLOT', slot, 'NODE', to_id)
            try:
                await from_client.cluster('SETSLOT', slot, 'NODE', to_id)
            except redis.exceptions.ResponseError:
                # In case it was the last slot.
                # Host without any slot is not a master
                pass
        finally:
            await from_client.aclose()
            await to_client.aclose()
        # if still not enough to beat flaps then try to SETSLOT on every node
        self.slots_by_node[from_node.get_address()].discard(slot)
        self.slots_by_node[to_node.get_address()].add(slot)

    async def _move_hash_slots(
        self,
        from_node: _RedisClusterNode,
        to_node: _RedisClusterNode,
        hash_slot_count,
    ):
        from_addr = from_node.get_address()
        from_slots = self.slots_by_node[from_addr]
        if len(from_slots) < hash_slot_count:
            raise Exception(
                f'Invalid number of slots to move {hash_slot_count}:{len(from_slots)}',
            )
        tasks = []
        for i in range(hash_slot_count):
            slot = from_slots.pop()
            tasks.append(self._move_slot(from_node, to_node, slot))
        await asyncio.gather(*tasks)

    async def _add_node_to_cluster(
        self,
        entry_node: _RedisClusterNode,
        new_node: _RedisClusterNode,
        replica=False,
    ):
        """Attach new_node to cluster of entry_node"""
        entry_address = entry_node.get_address()
        new_address = new_node.get_address()
        args = ['--cluster', 'add-node', new_address, entry_address]
        if replica:
            args += ['--cluster-slave']
        await _call_binary(_get_prefixed_path('bin', 'redis-cli'), *args)

    async def _wait_nodes_connect(self, ports: [tuple[int, int]]) -> bool:
        """Wait until every node connects to other nodes."""
        expected_ids = set()
        for port, _ in ports:
            myid = redis.Redis(port=port).cluster('myid').decode()
            expected_ids.add(myid)

        for port, _ in ports:
            client = aredis.Redis(port=port)

            async def is_ready() -> bool:
                ret = await client.cluster('nodes')
                if len(ret) != len(ports):
                    logging.warning(f'failed get nodes (wrong count) {ret}')
                    return False
                ids = set()
                for data in ret.values():
                    if data['flags'] == 'handshake':
                        logging.warning(f'failed get nodes (handshake) {ret}')
                        return False
                    ids.add(data['node_id'])
                if ids != expected_ids:
                    logging.warning(
                        f'failed get nodes (wrong ids) {ret} {ids} {expected_ids}',
                    )
                    return False
                return True

            await sync.wait(
                is_ready,
                relax_msg='Redis cluster is not up yet, waiting from _wait_nodes_connect()...',
            )
        return True

    async def _create_cluster(self, ports):
        HOSTS_PER_SHARD = REPLICA_COUNT + 1
        SHARD_COUNT = CLUSTER_MINIMUM_NODES_COUNT // HOSTS_PER_SHARD
        master_ports = ports[::HOSTS_PER_SHARD]
        SLOTS_COUNT = 16384
        SLOTS_PER_SHARD = SLOTS_COUNT // SHARD_COUNT
        SLOTS_REM = SLOTS_COUNT % SHARD_COUNT

        epoch = 0
        for client_port, _ in ports:
            redis.Redis(port=client_port).cluster('set-config-epoch', epoch)
            epoch += 1

        # meet each other
        for client_port, _ in ports:
            c = redis.Redis(port=client_port)
            for port, cport in ports:
                if port != client_port:
                    c.cluster(
                        'meet',
                        REDIS_CLUSTER_HOST,
                        str(port),
                        str(cport),
                    )

        # assign slots to masters
        for i in range(SHARD_COUNT):
            port = port = master_ports[i][0]
            c = redis.Redis(port=port)
            left = i * SLOTS_PER_SHARD
            rem = SLOTS_REM if i == SHARD_COUNT - 1 else 0
            right = (i + 1) * SLOTS_PER_SHARD + rem - 1
            c.cluster('addslotsrange', left, right)
            self.slots_by_node[f'{REDIS_CLUSTER_HOST}:{port}'] = {x for x in range(left, right + 1)}

        # Wait until every one now each other
        await self._wait_nodes_connect(ports)

        replica_ports = []
        # assign replicas to masters
        for shard_idx in range(SHARD_COUNT):
            master_port = master_ports[shard_idx][0]
            master_id = redis.Redis(port=master_port).cluster('myid')
            for replica in range(REPLICA_COUNT):
                replica_idx = shard_idx * HOSTS_PER_SHARD + replica + 1
                replicas_port = ports[replica_idx][0]
                replica_ports.append(replicas_port)
                replica_client = redis.Redis(port=replicas_port)
                try:
                    replica_client.cluster('replicate', master_id)
                except redis.exceptions.RedisError as e:
                    logger.error(
                        f'Failed to set replicate:'
                        f'replica:{replicas_port} '
                        f'master_id:{master_id} : {e} '
                        f'nodes:{replica_client.cluster("nodes")}',
                    )
                self.slots_by_node[f'{REDIS_CLUSTER_HOST}:{replicas_port}'] = self.slots_by_node[
                    f'{REDIS_CLUSTER_HOST}:{master_port}'
                ]
        await self._wait_cluster_nodes_ready(self.nodes, CLUSTER_MINIMUM_NODES_COUNT)

    async def _wait_cluster_nodes_known(self, nodes, expected_nodes_count) -> None:
        for node in nodes:

            async def check_ready():
                known_nodes = await self._count_known_hosts(node)
                if known_nodes != expected_nodes_count:
                    logger.warning(
                        f'Redis node {node.get_address()} is not ready yet '
                        f'({known_nodes} of {expected_nodes_count}), waiting...',
                    )
                    raise sync.NotReady()

            await sync.wait_until(check_ready, total_wait_seconds=TOTAL_WAIT_SECONDS)

    async def _wait_cluster_nodes_ready(self, nodes, expected_nodes_count) -> None:
        for node in nodes:

            async def is_ready():
                return await self._count_hosts_in_cluster(node) == expected_nodes_count

            await sync.wait(
                is_ready,
                relax_msg='Redis cluster is not up yet, waiting from _wait_cluster_nodes_ready()...',
                total_wait_seconds=TOTAL_WAIT_SECONDS,
            )

    async def _count_known_hosts(self, node):
        """
        Count hosts that are known to specified node, but may be not
        participating in cluster (does not have assigned slots)
        """
        try:
            client = node.get_async_client()
            return len(await client.cluster('NODES'))
        except redis.exceptions.ConnectionError:
            logger.warning(f'Failed to connect to server {node.get_address()}')
            return 0

    async def _count_hosts_in_cluster(self, node):
        """
        Count hosts that have assigned slots so they are actually part of
        the cluster
        """
        client = node.get_async_client()
        cluster_slots = await client.cluster('SLOTS')
        ports = set()
        for interval in cluster_slots:
            master = interval[2]
            replicas = interval[3:]
            master_port = master[1]
            ports.add(master_port)
            ports.update({replica[1] for replica in replicas})
        return len(ports)


class RedisClusterTopologyPlugin:
    def __init__(self):
        self.ports = [port_manager.get_port() for _ in range(CLUSTER_MINIMUM_NODES_COUNT)]
        self.cluster_ports = [port_manager.get_port() for _ in range(CLUSTER_MINIMUM_NODES_COUNT)]

    @pytest.fixture(scope='session')
    async def redis_cluster_topology_session(self):
        redis_cluster = RedisClusterTopology(
            self.ports,
            self.cluster_ports,
        )
        await redis_cluster.start()
        yield redis_cluster
        redis_cluster.cleanup()

    @pytest.fixture(scope='session')
    def redis_cluster_ports(self):
        return self.ports


@pytest.fixture
async def redis_cluster_topology(redis_cluster_topology_session):
    # remove shard if was previously added
    await redis_cluster_topology_session.remove_shard()
    # start nodes if some was previously stopped
    await redis_cluster_topology_session.start_all_nodes()
    return redis_cluster_topology_session


def pytest_configure(config):
    config.pluginmanager.register(
        RedisClusterTopologyPlugin(),
        '_redis_cluster_topology_no_docker_plugin',
    )
