import asyncio
import logging
import os
import os.path
import platform
import shutil
import signal
import subprocess
import time

from library.python import resource
import pytest
import redis
import redis.asyncio as aredis
import yatest.common
import yatest.common.network

CLUSTER_MINIMUM_NODES_COUNT = 6
REPLICA_COUNT = 1
MAX_CHECK_RETRIES = 10
RETRY_TIMEOUT = 5
REDIS_CLUSTER_HOST = '127.0.0.1'

logger = logging.getLogger(__name__)
port_manager = yatest.common.network.PortManager()


class RedisClusterTopologyError(Exception):
    pass


def _get_base_path() -> str:
    package_dir = (
        'taxi/uservices/userver/redis/'
        'functional_tests/pytest_redis_cluster_topology_plugin/package'
    )
    return yatest.common.build_path(package_dir)


def _get_prefixed_path(*path_parts: str) -> str:
    return os.path.join(_get_base_path(), *path_parts)


def _call_binary(binary_name: str, *args: str) -> None:
    logger.debug('Calling %s with %s', binary_name, args)
    proc = subprocess.Popen(
        [_get_prefixed_path('bin', binary_name), *args],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        env={
            'DYLD_LIBRARY_PATH'
            if platform.system() == 'Darwin'
            else 'LD_LIBRARY_PATH': _get_prefixed_path('lib'),
        },
    )
    try:
        outs, errs = proc.communicate(timeout=15)
        logger.info(outs)
        logger.warning(errs)
    except subprocess.TimeoutExpired:
        proc.kill()
        outs, errs = proc.communicate()
        logger.info(outs)
        logger.warning(errs)


def _get_data_directory():
    return yatest.common.work_path('_redis_cluster_topology')


def _do_tries(
        f, tries, timeout, on_single_fail_message=None, on_fail_message=None,
):
    for _ in range(tries):
        if f():
            break
        if on_fail_message:
            logger.warning(on_single_fail_message)
        time.sleep(timeout)
    else:
        raise Exception(on_single_fail_message)


class _RedisClusterNode:
    def __init__(self, host: str, port: int, cluster_port: int):
        config_name = 'cluster.conf'
        self.host = host
        self.port = port
        self.cluster_port = cluster_port
        self.data_directory = os.path.join(
            _get_data_directory(), f'redis_{host}:{port}',
        )
        self.pid_path = os.path.join(self.data_directory, 'redis.pid')
        self.config_path = os.path.join(self.data_directory, config_name)
        self.log_path = os.path.join(self.data_directory, 'redis.log')
        shutil.rmtree(self.data_directory, ignore_errors=True)
        os.makedirs(self.data_directory, exist_ok=True)
        self._write_config()
        self.started = False

    def get_info(self, section=None):
        client = self.get_client()
        ret = client.info(section)
        return ret

    def get_client(self) -> redis.Redis:
        return redis.Redis(host=self.host, port=self.port)

    def get_async_client(self) -> aredis.Redis:
        return aredis.Redis(host=self.host, port=self.port)

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

    def start(self):
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
        _call_binary('redis-server', *args)
        self.wait_ready()
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
            return
        self.started = False

    def check_instance(self):
        with self.get_client() as client:
            try:
                client.ping()
            except redis.exceptions.ConnectionError as exc:
                logger.warning('Cannot get instance: %s', str(exc))
                return False
        return True

    def wait_ready(self):
        for _ in range(MAX_CHECK_RETRIES):
            if self.check_instance():
                logger.info(f'redis {self.get_address()} successfully pinged')
                break
            logger.warning('Redis cluster is not up yet, waiting')
            time.sleep(RETRY_TIMEOUT)
        else:
            raise RedisClusterTopologyError(
                f'Redis cluster not ready after'
                f' {MAX_CHECK_RETRIES} retries',
            )

    def _write_config(self):
        resource_name = 'redis_cluster_config'
        content = resource.find(resource_name)
        assert (
            content is not None
        ), f'Could not find resource for {resource_name}'
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
        all_ports = [
            (port, cport) for port, cport in zip(ports, cluster_ports)
        ]
        self.nodes = [
            _RedisClusterNode(REDIS_CLUSTER_HOST, port, cluster_port)
            for port, cluster_port in all_ports
        ]
        self.nodes_by_names = {node.get_address(): node for node in self.nodes}
        self.added_master = None
        self.added_replica = None
        self.start_all_nodes()
        self.slots_by_node = {}
        self._create_cluster(all_ports)
        self.client = redis.RedisCluster(
            startup_nodes=[
                redis.cluster.ClusterNode('localhost', port) for port in ports
            ],
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
        self._wait_cluster_nodes_known(self.nodes, 6)
        self._wait_cluster_nodes_ready(self.nodes, 6)

        # create 2 nodes
        new_master = _RedisClusterNode(
            REDIS_CLUSTER_HOST,
            port_manager.get_port(),
            port_manager.get_port(),
        )
        self.added_master = new_master
        new_master.start()
        slot_set = set()
        self.slots_by_node[new_master.get_address()] = slot_set

        new_replica = _RedisClusterNode(
            REDIS_CLUSTER_HOST,
            port_manager.get_port(),
            port_manager.get_port(),
        )
        self.added_replica = new_replica
        new_replica.start()
        self.slots_by_node[new_replica.get_address()] = slot_set

        # add new nodes to existing cluster
        old_masters = self.get_masters()
        self._add_node_to_cluster(self.nodes[0], new_master)
        self._wait_cluster_nodes_known(old_masters, len(self.nodes) + 1)

        self._add_node_to_cluster(new_master, new_replica, replica=True)
        self._wait_cluster_nodes_known(old_masters, len(self.nodes) + 2)

        nodes = (
            old_masters + [new_master] + self.get_replicas() + [new_replica]
        )
        # wait until nodes know each other
        self._wait_cluster_nodes_known(nodes, len(nodes))

        # move part of hash slots from each of old master to new master
        slot_count = 16384 // 3 // 4
        for master in old_masters:
            await self._move_hash_slots(master, new_master, slot_count)

        # wait until all nodes know new cluster configuration
        self._wait_cluster_nodes_ready(nodes, len(nodes))

    async def remove_shard(self):
        """
        Removes forth shard added by add_shard method
        """
        if self.added_master is None and self.added_replica is None:
            # already in desired state
            return

        # Validate state before proceed
        self._wait_cluster_nodes_known(self.nodes, 8)
        self._wait_cluster_nodes_ready(self.nodes, 8)
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
                'Failed to notify all cluster nodes about deleted'
                ' nodes within 1 minute',
            )

        self._wait_cluster_nodes_ready(original_masters, 6)
        self._wait_cluster_nodes_ready(original_replicas, 6)
        self._wait_cluster_nodes_known(original_masters, 6)
        self._wait_cluster_nodes_known(original_replicas, 6)

        new_master.stop()
        new_replica.stop()
        self.added_master = None
        self.added_replica = None

        # wait until just removed node will be removed from ban-list to allow
        # further addition of this node
        await asyncio.sleep(60)

    def start_all_nodes(self):
        for node in self.nodes:
            node.start()
        if self.added_master:
            self.added_master.start()
        if self.added_replica:
            self.added_replica.start()

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
            timeout = 1000
            db = 0
            while True:
                keys = await from_client.cluster('GETKEYSINSLOT', slot, batch)
                if not keys:
                    break
                await from_client.migrate(
                    REDIS_CLUSTER_HOST, to_node.port, keys, db, timeout,
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
                f'Invalid number of slots to move '
                f'{hash_slot_count}:{len(from_slots)}',
            )
        tasks = []
        for i in range(hash_slot_count):
            slot = from_slots.pop()
            tasks.append(self._move_slot(from_node, to_node, slot))
        await asyncio.gather(*tasks)

    def _add_node_to_cluster(
            self,
            entry_node: _RedisClusterNode,
            new_node: _RedisClusterNode,
            replica=False,
    ):
        """ Attach new_node to cluster of entry_node """
        entry_address = entry_node.get_address()
        new_address = new_node.get_address()
        args = ['--cluster', 'add-node', new_address, entry_address]
        if replica:
            args += ['--cluster-slave']
        _call_binary(_get_prefixed_path('bin', 'redis-cli'), *args)

    def _wait_nodes_connect(self, ports: [tuple[int, int]]) -> bool:
        """ Wait until every node connects to other nodes.  """
        expected_ids = set()
        for port, _ in ports:
            myid = redis.Redis(port=port).cluster('myid').decode()
            expected_ids.add(myid)

        for port, _ in ports:

            def f():
                client = redis.Redis(port=port)
                ret = client.cluster('nodes')
                if len(ret) != len(ports):
                    logging.warning(f'failed get nodes (wrong count) {ret}')
                    return False
                ids = set()
                for node, data in ret.items():
                    if data['flags'] == 'handshake':
                        logging.warning(f'failed get nodes (handshake) {ret}')
                        return False
                    ids.add(data['node_id'])
                if ids != expected_ids:
                    logging.warning(
                        f'failed get nodes (wrong ids) '
                        f'{ret} {ids} {expected_ids}',
                    )
                    return False

                return True

            _do_tries(
                f,
                MAX_CHECK_RETRIES,
                RETRY_TIMEOUT,
                'Redis cluster is not up yet, waiting...',
                f'Redis cluster not ready after'
                f' {MAX_CHECK_RETRIES} retries',
            )
        return True

    def _create_cluster(self, ports):
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
                        'meet', REDIS_CLUSTER_HOST, str(port), str(cport),
                    )

        # assign slots to masters
        for i in range(SHARD_COUNT):
            port = port = master_ports[i][0]
            c = redis.Redis(port=port)
            left = i * SLOTS_PER_SHARD
            rem = SLOTS_REM if i == SHARD_COUNT - 1 else 0
            right = (i + 1) * SLOTS_PER_SHARD + rem - 1
            c.cluster('addslotsrange', left, right)
            self.slots_by_node[f'{REDIS_CLUSTER_HOST}:{port}'] = {
                x for x in range(left, right + 1)
            }

        # Wait until every one now each other
        self._wait_nodes_connect(ports)

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
                self.slots_by_node[
                    f'{REDIS_CLUSTER_HOST}:{replicas_port}'
                ] = self.slots_by_node[f'{REDIS_CLUSTER_HOST}:{master_port}']
        self._wait_cluster_nodes_ready(self.nodes, CLUSTER_MINIMUM_NODES_COUNT)

    def _wait_cluster_nodes_known(self, nodes, expected_nodes_count) -> bool:
        for node in nodes:
            for _ in range(MAX_CHECK_RETRIES):
                known_nodes = self._count_known_hosts(node)
                if known_nodes == expected_nodes_count:
                    break
                logger.warning(
                    f'Redis node {node.get_address()} is not ready yet '
                    f'({known_nodes} of {expected_nodes_count}), waiting...',
                )
                time.sleep(RETRY_TIMEOUT)
            else:
                raise RedisClusterTopologyError(
                    f'Redis cluster not ready after'
                    f' {MAX_CHECK_RETRIES} retries',
                )
        return True

    def _wait_cluster_nodes_ready(self, nodes, expected_nodes_count) -> bool:
        for node in nodes:
            for _ in range(MAX_CHECK_RETRIES):
                if self._count_hosts_in_cluster(node) == expected_nodes_count:
                    break
                logger.warning('Redis cluster is not up yet, waiting...')
                time.sleep(RETRY_TIMEOUT)
            else:
                raise RedisClusterTopologyError(
                    f'Redis cluster not ready after'
                    f' {MAX_CHECK_RETRIES} retries',
                )
        return True

    def _count_known_hosts(self, node):
        """
        Count hosts that are known to specified node, but may be not
        participating in cluster (does not have assigned slots)
        """
        try:
            client = node.get_client()
            return len(client.cluster('NODES'))
        except redis.exceptions.ConnectionError:
            return 0

    def _count_hosts_in_cluster(self, node):
        """
        Count hosts that have assigned slots so they are actually part of
        the cluster
        """
        client = node.get_client()
        cluster_slots = client.cluster('SLOTS')
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
        self.ports = [
            port_manager.get_port() for _ in range(CLUSTER_MINIMUM_NODES_COUNT)
        ]
        self.cluster_ports = [
            port_manager.get_port() for _ in range(CLUSTER_MINIMUM_NODES_COUNT)
        ]
        self.redis_cluster = RedisClusterTopology(
            self.ports, self.cluster_ports,
        )

    @pytest.fixture(scope='session')
    def redis_cluster_topology_session(self):
        return self.redis_cluster

    @pytest.fixture
    async def redis_cluster_topology(self):
        # remove shard if was previously added
        await self.redis_cluster.remove_shard()
        # start nodes if some was previously stopped
        self.redis_cluster.start_all_nodes()
        return self.redis_cluster

    @pytest.fixture(scope='session')
    def redis_cluster_ports(self):
        return self.ports


def pytest_configure(config):
    config.pluginmanager.register(
        RedisClusterTopologyPlugin(),
        '_redis_cluster_topology_no_docker_plugin',
    )
