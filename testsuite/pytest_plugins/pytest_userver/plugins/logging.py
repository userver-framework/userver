import io
import pathlib
import sys
import threading
import time
import typing

import pytest

from ..utils import colorize


class LogFile:
    def __init__(self, path: pathlib.Path):
        self.path = path
        self.update_position()

    def update_position(self):
        try:
            st = self.path.stat()
        except FileNotFoundError:
            pos = 0
        else:
            pos = st.st_size
        self.position = pos
        return pos

    def readlines(
            self,
            eof_handler: typing.Optional[typing.Callable[[], bool]] = None,
    ):
        first_skipped = False
        for line in _raw_line_reader(
                self.path, self.position, eof_handler=eof_handler,
        ):
            # userver does not give any gurantees about log file encoding
            line = line.decode(encoding='utf-8', errors='ignore')
            if not first_skipped:
                first_skipped = True
                if not line.startswith('tskv '):
                    continue
            if not line.endswith('\n'):
                continue
            yield line
        self.update_position()


class LiveLogHandler:
    def __init__(self, *, colorize_factory, delay: float = 0.05):
        self._colorize_factory = colorize_factory
        self._threads = {}
        self._exiting = False
        self._delay = delay

    def register_log_file(self, path: pathlib.Path):
        if path in self._threads:
            return
        thread = threading.Thread(target=self._logreader_thread, args=(path,))
        self._threads[path] = thread
        thread.start()

    def join(self, timeout: float = 10):
        self._exiting = True
        for thread in self._threads.values():
            thread.join(timeout=timeout)

    def _logreader_thread(self, path: pathlib.Path):
        # wait for file to appear
        while not path.exists():
            if self._eof_handler():
                return
        colorizer = self._colorize_factory()
        log_file = LogFile(path)
        for line in log_file.readlines(eof_handler=self._eof_handler):
            line = line.rstrip('\r\n')
            line = colorizer(line)
            if line:
                self._write_logline(line)

    def _write_logline(self, line: str):
        print(line, file=sys.stderr)

    def _eof_handler(self) -> bool:
        if self._exiting:
            return True
        time.sleep(self._delay)
        return False


class UserverLoggingPlugin:
    _log_file: typing.Optional[LogFile] = None
    _live_logs: typing.Optional[LiveLogHandler] = None

    def __init__(self, *, colorize_factory, live_logs_enabled: bool = False):
        self._colorize_factory = colorize_factory
        if live_logs_enabled:
            self._live_logs = LiveLogHandler(colorize_factory=colorize_factory)

    def pytest_sessionfinish(self, session):
        if self._live_logs:
            self._live_logs.join()

    def pytest_runtest_setup(self, item):
        if self._log_file:
            self._log_file.update_position()

    @pytest.hookimpl(wrapper=True)
    def pytest_runtest_teardown(self, item):
        yield from self._userver_log_dump(item, 'teardown')

    def register_log_file(self, path: pathlib.Path):
        self._log_file = LogFile(path)
        if self._live_logs:
            self._live_logs.register_log_file(path)

    def _userver_log_dump(self, item, when):
        try:
            yield
        except Exception:
            self._userver_report_attach(item, when)
            raise
        if item.utestsuite_report.failed:
            self._userver_report_attach(item, when)

    def _userver_report_attach(self, item, when):
        if not self._log_file:
            return
        report = io.StringIO()
        colorizer = self._colorize_factory()
        for line in self._log_file.readlines():
            line = line.rstrip('\r\n')
            line = colorizer(line)
            if line:
                report.write(line)
                report.write('\n')
        value = report.getvalue()
        if value:
            item.add_report_section(when, 'userver/log', value)


@pytest.fixture(scope='session')
def _uservice_logfile_path(pytestconfig, service_tmpdir) -> pathlib.Path:
    userver_logging: UserverLoggingPlugin
    userver_logging = pytestconfig.pluginmanager.get_plugin('userver_logging')
    path = pytestconfig.option.service_logs_file
    if path is None:
        path = service_tmpdir / 'service.log'
    with open(path, 'w+') as fp:
        fp.truncate()
    userver_logging.register_log_file(path)
    return path


def pytest_configure(config):
    if config.option.capture == 'no' and config.option.showcapture in (
            'all',
            'log',
    ):
        live_logs_enabled = True
    else:
        live_logs_enabled = False

    pretty_logs = config.option.service_logs_pretty
    colors_enabled = _should_enable_color(config)
    verbose = pretty_logs == 'verbose'

    def colorize_factory():
        if pretty_logs:
            colorizer = colorize.Colorizer(
                verbose=verbose, colors_enabled=colors_enabled,
            )
            return colorizer.colorize_line

        def handle_line(line):
            return line

        return handle_line

    plugin = UserverLoggingPlugin(
        colorize_factory=colorize_factory, live_logs_enabled=live_logs_enabled,
    )
    config.pluginmanager.register(plugin, 'userver_logging')


def _should_enable_color(pytestconfig) -> bool:
    option = getattr(pytestconfig.option, 'color', 'no')
    if option == 'yes':
        return True
    if option == 'auto':
        return sys.stderr.isatty()
    return False


def _raw_line_reader(
        path: pathlib.Path,
        position: int = 0,
        eof_handler: typing.Optional[typing.Callable[[], bool]] = None,
        bufsize: int = 16384,
):
    buf = b''
    with path.open('rb') as fp:
        fp.seek(position)
        while True:
            chunk = fp.read(bufsize)
            if not chunk:
                if not eof_handler:
                    break
                if eof_handler():
                    break
            else:
                buf += chunk
                lines = buf.splitlines(keepends=True)
                if not lines[-1].endswith(b'\n'):
                    buf = lines[-1]
                    del lines[-1]
                else:
                    buf = b''
                yield from lines
