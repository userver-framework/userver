import asyncio  # noqa: I001
import asyncio.subprocess
import logging
import os
import os.path
import platform
import re
import shutil
import signal
import subprocess
import socket
import time
from typing import Any
from typing import TypeAlias

import pytest
import redis
import redis.asyncio as aredis
import yatest.common

from library.python import port_manager
from library.python import resource
import pytest_userver.utils.sync as sync
from pytest_userver import chaos

TOTAL_WAIT_SECONDS = 180
CLUSTER_MINIMUM_NODES_COUNT = 6
REPLICA_COUNT = 1
REDIS_CLUSTER_HOST = '127.0.0.1'

logger = logging.getLogger(__name__)
pm = port_manager.PortManager()


EvLoop: TypeAlias = Any
Socket: TypeAlias = socket.socket


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


class ProxyInterceptor:
    def __init__(self, all_hosts: list[tuple[int, int, int]], encoding='utf-8'):
        """
        all_hosts - list of real instance hostname, real instance port, real instance cluster port, proxy port
        """

        self._list = all_hosts
        self._patterns = [(re.compile(pattern), repl) for pattern, repl in self._make_substitute_patterns(all_hosts)]
        self._encoding = encoding

    async def __call__(
        self,
        loop: EvLoop,
        socket_from: Socket,
        socket_to: Socket,
    ) -> None:
        data = await loop.sock_recv(socket_from, 4096)
        if not data:
            raise chaos.ConnectionClosedError()
        try:
            data = self._transform(data)
        except UnicodeError:
            pass
        await loop.sock_sendall(socket_to, data)

    def _transform(self, data: bytes) -> bytes:
        decoded = data.decode(self._encoding)
        decoded = self._process_cluster_nodes(decoded)
        for pattern, repl in self._patterns:
            decoded = pattern.sub(repl, decoded)
        return decoded.encode(self._encoding)

    def _process_cluster_nodes(self, data: str) -> str:
        """
        detects if string is similar to:
        ```
        +PONG <<Can be +OK and maybe other strings or maybe nothing>>
        $727
        76f39983c800af9393028d01792eed3984ea7a44 127.0.0.1:6380@7380 myself,master - 0 0 1 connected 0-5460
        b79724912ccbef350b62657c42a4388fbf77f352 127.0.0.1:6384@7384 slave 404d3cdd247f224c53001c2bb188694b455046e5 0 1779094106410 2 connected
        404d3cdd247f224c53001c2bb188694b455046e5 127.0.0.1:6381@7381 master - 0 1779094106410 2 connected 5461-10922
        d42049e80952f735239c90bdab8d0f3c0722c10c 127.0.0.1:6385@7385 slave 3830472185ccafc92dc3cd615f1b0755f883b20d 0 1779094106410 3 connected
        a26a24058eedc5e0f9b0fff035069abaa4eaa02c 127.0.0.1:6383@7383 slave 76f39983c800af9393028d01792eed3984ea7a44 0 1779094106410 1 connected
        3830472185ccafc92dc3cd615f1b0755f883b20d 127.0.0.1:6382@7382 master - 0 1779094106410 3 connected 10923-16383
        ```
        If so, substitute the ports with proxy ports and adjust the message length accordingly.
        """
        if not all(word in data for word in ['myself', 'connected', 'master', 'slave', '@', '\r\n']):
            return data

        # need to find line with payload size addjust to corrected message
        # do not loose lines previous to message size and size itself
        lines = data.split('\r\n')
        # Find the bulk string length line (starts with $)
        # It usually appears right after the status line (+PONG, etc.)
        # or at the beginning if no status line.
        # The structure is roughly:
        # Line 0: Status (optional)
        # Line 1: Bulk String Length ($<len>)
        # Line 2..N: Content

        # We need to identify the index of the bulk string length line.
        # It starts with '$'.
        length_index = -1
        for i, line in enumerate(lines):
            if line.startswith('$') and '@' in lines[i + 1]:
                length_index = i
                break

        if length_index == -1:
            # No bulk string found, return original
            return data

        last_line_index = length_index + 2
        # The content starts at length_index + 1
        content_lines = lines[length_index + 1 : last_line_index]
        content_str = '\r\n'.join(content_lines)

        # Perform substitutions on the content
        # We use the patterns defined in _make_substitute_patterns
        # Note: self._patterns is a list of (compiled_regex, replacement_string)
        for pattern, repl in self._patterns:
            content_str = pattern.sub(repl, content_str)

        # Reconstruct the data
        # Calculate new length
        new_length = len(content_str)

        # Construct new lines
        # Keep everything before the content
        new_lines = lines[:length_index]
        # Update the length line
        new_lines.append(f'${new_length}')
        # Add the modified content
        new_lines.append(content_str)

        new_lines = new_lines + lines[last_line_index:]

        # log original and final data
        return '\r\n'.join(new_lines)

    def _make_substitute_patterns(self, all_hosts: list[tuple[int, int, int]]) -> list[tuple[str, str]]:
        """
        returns list of tuples(regex substitute pattern, string to replace) for single host to mock responses from redis commands (cluster nodes, cluster
        shards, cluster slots)
        """
        patterns = []
        # all_hosts: list of (real_port, real_cluster_port, proxy_port)

        host = REDIS_CLUSTER_HOST
        for port, cluster_port, proxy_port in all_hosts:
            # Pattern for CLUSTER NODES is NOT USED because it is harder than just replace.
            # Matches: <h_host>:<h_port>@<h_cluster_port>
            # Example: 127.0.0.1:6380@7380
            # We replace with: <h_host>:<h_proxy_port>@<h_cluster_port>
            pattern = rf'\b{host}:{port}@{cluster_port}'
            replacement = rf'{host}:{proxy_port}@{cluster_port}'
            patterns.append((pattern, replacement))

            # Pattern for CLUSTER SLOTS
            # Format: *4 \n $<len> \n <ip> \n :<port> \n $<len> \n <id> \n ...
            # Pattern for Port integer: r':' + str(h_port) + r'\r\n'
            # Replacement: r':' + str(h_proxy_port) + r'\r\n'
            patterns.append((
                f'\\${len(host)}\r\n{host}\r\n:{port}\r\n',
                f'${len(host)}\r\n{host}\r\n:{proxy_port}\r\n',
            ))

            # Pattern for port in CLUSTER SHARDS
            # The key is "port" (4 bytes).
            # The value is an integer, e.g., :6382.
            # We want to replace :6382 with :<proxy_port>.
            # We must ensure we don't replace other integers.
            # The key "port" is unique enough in this context.
            patterns.append((rf'port\r\n:{port}\r\n', rf'port\r\n:{proxy_port}\r\n'))

        return patterns


class _RedisClusterNode:
    def __init__(self, host: str, port: int, cluster_port: int, proxy_port: int):
        config_name = 'cluster.conf'
        self.host = host
        self.port = port
        self.proxy_port = proxy_port
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
        self.proxy = None
        self.started = False

        self.connection_pool = aredis.ConnectionPool.from_url(f'redis://{host}:{port}')

    async def _add_proxy(self, all_hosts: list[tuple[int, int, int]]):
        """
        Add proxy to node wich substitutes all real nodes' ips and ports to specified.
        all_hosts - list of real instance hostname, real instance port, real instance cluster port, proxy port
        """
        gate_config = chaos.GateRoute(
            name=f'{self.host}:{self.proxy_port}->{self.port}',
            host_for_client=REDIS_CLUSTER_HOST,
            port_for_client=self.proxy_port,
            host_to_server=REDIS_CLUSTER_HOST,
            port_to_server=self.port,
        )
        self.proxy: chaos.TcpGate = chaos.TcpGate(gate_config)
        await self.proxy.set_to_client_interceptor(ProxyInterceptor(all_hosts))
        await self.proxy.to_server_pass()
        self.proxy.start()

    def get_info(self, section=None):
        client = self.get_client()
        ret = client.info(section)
        return ret

    def get_client(self) -> redis.Redis:
        return redis.Redis(host=self.host, port=self.port)

    def get_async_client(self) -> aredis.Redis:
        return aredis.Redis(connection_pool=self.connection_pool)

    def _get_nodes_by_role(self, role) -> list[str]:
        if self.version < '8.0.0':
            return []

        try:
            client = self.get_client()
            cluster_shards = client.cluster('SHARDS')
        except (BaseException, redis.exceptions.ConnectionError):
            return []

        ret = []
        for shard in cluster_shards:
            nodes = shard[3]  # "nodes" field
            for node in nodes:
                # node is a list of [key, value, key, value, ...] - like a map
                node_dict = {}
                for i in range(0, len(node), 2):
                    node_dict[node[i]] = node[i + 1]
                # Check if this node matches the requested role
                node_role = node_dict.get(b'role', b'').decode()
                if node_role == role:
                    ip = node_dict.get(b'ip', b'').decode()
                    port = node_dict.get(b'port')
                    if ip and port:
                        ret.append(f'{ip}:{port}')
        ret.sort()
        return ret

    def get_primary_addresses(self) -> list[str]:
        if ret := self._get_nodes_by_role('master'):
            return ret

        try:
            client = self.get_client()
            cluster_slots = client.cluster('SLOTS')
        except (BaseException, redis.exceptions.ConnectionError):
            return []

        ret = []
        for interval in cluster_slots:
            master = interval[2]
            ret.append(f'{master[0].decode()}:{master[1]}')
        ret.sort()
        return ret

    def get_replica_addresses(self) -> list[str]:
        if ret := self._get_nodes_by_role('replica'):
            return ret

        try:
            client = self.get_client()
            cluster_slots = client.cluster('SLOTS')
        except (BaseException, redis.exceptions.ConnectionError):
            return []

        ret = []
        for interval in cluster_slots:
            replicas = interval[3:]
            for replica in replicas:
                ret.append(f'{replica[0].decode()}:{replica[1]}')
        ret.sort()
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
        if self.proxy:
            self.proxy.start()
        self.started = True

        # string version from INFO response
        info = self.get_info()
        self.version = info.get('valkey_version', info.get('redis_version'))

    async def stop(self):
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
        if self.proxy:
            await self.proxy.stop()
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
    def __init__(self, ports, cluster_ports, proxy_ports):
        self.ports = ports
        self.all_ports = [
            (port, cport, proxy_port) for port, cport, proxy_port in zip(ports, cluster_ports, proxy_ports, strict=True)
        ]
        self.nodes = [
            _RedisClusterNode(REDIS_CLUSTER_HOST, port, cluster_port, proxy_port)
            for port, cluster_port, proxy_port in self.all_ports
        ]
        self.nodes_by_names = {node.get_address(): node for node in self.nodes}
        self.added_master = None
        self.added_replica = None
        self.slots_by_node = {}
        self.client = None

    async def start(self):
        for node in self.nodes:
            await node._add_proxy(self.all_ports)
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

        # create 2 nodes.
        # Additional shard has no proxy. so we do not call _add_proxy.
        new_master = _RedisClusterNode(
            REDIS_CLUSTER_HOST,
            pm.get_port(),
            pm.get_port(),
            pm.get_port(),
        )
        self.added_master = new_master
        await new_master.start()
        slot_set = set()
        self.slots_by_node[new_master.get_address()] = slot_set

        new_replica = _RedisClusterNode(
            REDIS_CLUSTER_HOST,
            pm.get_port(),
            pm.get_port(),
            pm.get_port(),
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

        await new_master.stop()
        await new_replica.stop()
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

    async def cleanup(self):
        for node in self.nodes:
            await node.stop()
        if self.added_master:
            await self.added_master.stop()
        if self.added_replica:
            await self.added_replica.stop()

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

    async def _wait_nodes_connect(self, ports: [tuple[int, int, int]]) -> bool:
        """Wait until every node connects to other nodes."""
        expected_ids = set()
        for port, _, _ in ports:
            myid = redis.Redis(port=port).cluster('myid').decode()
            expected_ids.add(myid)

        for port, _, _ in ports:
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
        for client_port, _, _ in ports:
            redis.Redis(port=client_port).cluster('set-config-epoch', epoch)
            epoch += 1

        # meet each other
        for client_port, _, _ in ports:
            c = redis.Redis(port=client_port)
            for port, cport, _ in ports:
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
        self.ports = [pm.get_port() for _ in range(CLUSTER_MINIMUM_NODES_COUNT)]
        self.cluster_ports = [pm.get_port() for _ in range(CLUSTER_MINIMUM_NODES_COUNT)]
        self.proxy_ports = [pm.get_port() for _ in range(CLUSTER_MINIMUM_NODES_COUNT)]

    @pytest.fixture(scope='session')
    async def redis_cluster_topology_session(self):
        redis_cluster = RedisClusterTopology(self.ports, self.cluster_ports, self.proxy_ports)
        await redis_cluster.start()
        yield redis_cluster
        await redis_cluster.cleanup()

    @pytest.fixture(scope='session')
    def redis_cluster_ports(self):
        return self.ports

    @pytest.fixture(scope='session')
    def redis_cluster_proxy_ports(self):
        return self.proxy_ports


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
