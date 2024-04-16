"""
Capture and work with logs.
"""

# pylint: disable=redefined-outer-name
import asyncio
import contextlib
import enum
import sys
import typing

import pytest

from testsuite.utils import callinfo
from testsuite.utils import compat
from testsuite.utils import net as net_utils

from ..utils import tskv

USERVER_CONFIG_HOOKS = ['_userver_config_logs_capture']
DEFAULT_PORT = 2211


class LogLevel(enum.Enum):
    TRACE = 0
    DEBUG = 1
    INFO = 2
    WARNING = 3
    ERROR = 4
    CRITICAL = 5
    NONE = 6

    @classmethod
    def from_string(cls, level: str) -> 'LogLevel':
        return cls[level.upper()]


class CapturedLogs:
    def __init__(self, *, log_level: str) -> None:
        self._log_level = LogLevel.from_string(log_level)
        self._logs: typing.List[tskv.TskvRow] = []
        self._subscribers: typing.List = []

    async def publish(self, row: tskv.TskvRow) -> None:
        self._logs.append(row)
        for query, callback in self._subscribers:
            if _match_entry(row, query):
                await callback(**row)

    def select(self, **query) -> typing.List[tskv.TskvRow]:
        level = query.get('level')
        if level:
            log_level = LogLevel[level]
            if log_level.value < self._log_level.value:
                raise RuntimeError(
                    f'Requested log level={log_level.name} is lower than '
                    f'service log level {self._log_level.name}',
                )
        result = []
        for row in self._logs:
            if _match_entry(row, query):
                result.append(row)
        return result

    def subscribe(self, **query):
        def decorator(func):
            decorated = callinfo.acallqueue(func)
            self._subscribers.append((query, decorated))
            return decorated

        return decorator


class CaptureControl:
    def __init__(self, *, log_level: str):
        self.default_log_level = log_level
        self._capture: typing.Optional[CapturedLogs] = None
        self._tasks = []

    @compat.asynccontextmanager
    async def start_capture(
            self,
            *,
            log_level: typing.Optional[str] = None,
            timeout: float = 10.0,
    ):
        if self._capture:
            yield self._capture
            return

        if not log_level:
            log_level = self.default_log_level

        self._capture = CapturedLogs(log_level=log_level)
        try:
            yield self._capture
        finally:
            self._capture = None
            if self._tasks:
                _, pending = await asyncio.wait(self._tasks, timeout=timeout)
                self._tasks = []
                if pending:
                    raise RuntimeError(
                        'Timedout while waiting for capture task to finish',
                    )

    @compat.asynccontextmanager
    async def start_server(self, *, sock, loop=None):
        extra = {}
        if sys.version_info < (3, 8):
            extra['loop'] = loop
        server = await asyncio.start_server(
            self._handle_client, sock=sock, **extra,
        )
        try:
            yield server
        finally:
            server.close()
            await server.wait_closed()

    async def _handle_client(self, reader, writer):
        async def log_reader():
            with contextlib.closing(writer):
                async for line in reader:
                    if self._capture:
                        row = tskv.parse_line(line.decode('utf-8'))
                        await self._capture.publish(row)

        self._tasks.append(asyncio.create_task(log_reader()))


def pytest_addoption(parser):
    group = parser.getgroup('logs-capture')
    group.addoption(
        '--logs-capture-port',
        type=int,
        default=0,
        help='Port to bind logs-capture server to.',
    )
    group.addoption(
        '--logs-capture-host',
        default='localhost',
        help='Host to bind logs-capture server to.',
    )


@pytest.fixture(scope='session')
def userver_log_capture(_userver_capture_control, _userver_capture_server):
    return _userver_capture_control


@pytest.fixture(scope='session')
def _userver_capture_control(userver_log_level):
    return CaptureControl(log_level=userver_log_level)


@pytest.fixture(scope='session')
def _userver_log_capture_socket(pytestconfig):
    host = pytestconfig.option.logs_capture_host
    port = pytestconfig.option.logs_capture_port
    if pytestconfig.option.service_wait or pytestconfig.option.service_disable:
        port = port or DEFAULT_PORT
    with net_utils.bind_socket(host, port) as socket:
        yield socket


@pytest.fixture(scope='session')
async def _userver_capture_server(
        _userver_capture_control: CaptureControl,
        _userver_log_capture_socket,
        loop,
):
    async with _userver_capture_control.start_server(
            sock=_userver_log_capture_socket, loop=loop,
    ) as server:
        yield server


@pytest.fixture(scope='session')
def _userver_config_logs_capture(_userver_log_capture_socket):
    def patch_config(config, _config_vars) -> None:
        sockname = _userver_log_capture_socket.getsockname()
        logging_config = config['components_manager']['components']['logging']
        default_logger = logging_config['loggers']['default']
        # Other formats are not yet supported by log-capture.
        default_logger['format'] = 'tskv'
        default_logger['testsuite-capture'] = {
            'host': sockname[0],
            'port': sockname[1],
        }

    return patch_config


def _match_entry(row: tskv.TskvRow, query) -> bool:
    for key, value in query.items():
        if row.get(key) != value:
            return False
    return True
