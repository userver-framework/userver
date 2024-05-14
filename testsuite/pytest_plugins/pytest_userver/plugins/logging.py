import io
import logging
import pathlib
import sys
import threading
import time
import typing

import pytest

from pytest_userver.utils import colorize

logger = logging.getLogger(__name__)


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
            line = line.decode(encoding='utf-8', errors='backslashreplace')
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

    def register_logfile(self, path: pathlib.Path):
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
        logfile = LogFile(path)
        for line in logfile.readlines(eof_handler=self._eof_handler):
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
    _live_logs = None

    def __init__(self, *, colorize_factory, live_logs_enabled: bool = False):
        self._colorize_factory = colorize_factory
        self._logs = {}
        if live_logs_enabled:
            self._live_logs = LiveLogHandler(colorize_factory=colorize_factory)

    def pytest_sessionfinish(self, session):
        if self._live_logs:
            self._live_logs.join()

    def pytest_runtest_setup(self, item):
        for logfile in self._logs.values():
            logfile.update_position()

    @pytest.hookimpl(wrapper=True)
    def pytest_runtest_teardown(self, item):
        yield from self._userver_log_dump(item, 'teardown')

    def register_logfile(self, path: pathlib.Path, title: str):
        logger.info('Watching service log file: %s', path)
        logfile = LogFile(path)
        self._logs[path, title] = logfile
        if self._live_logs:
            self._live_logs.register_logfile(path)

    def _userver_log_dump(self, item, when):
        try:
            yield
        except Exception:
            self._userver_report_attach(item, when)
            raise
        if item.utestsuite_report.failed:
            self._userver_report_attach(item, when)

    def _userver_report_attach(self, item, when):
        for (_, title), logfile in self._logs.items():
            self._userver_report_attach_log(logfile, item, when, title)

    def _userver_report_attach_log(self, logfile: LogFile, item, when, title):
        report = io.StringIO()
        colorizer = self._colorize_factory()
        for line in logfile.readlines():
            line = line.rstrip('\r\n')
            line = colorizer(line)
            if line:
                report.write(line)
                report.write('\n')
        value = report.getvalue()
        if value:
            item.add_report_section(when, title, value)


@pytest.fixture(scope='session')
def service_logfile_path(
        pytestconfig, service_tmpdir: pathlib.Path,
) -> typing.Optional[pathlib.Path]:
    """
    Holds optional service logfile path. You may want to override this
    in your service.

    By default returns value of --service-logs-file option or creates
    temporary file.
    """
    if pytestconfig.option.service_logs_file:
        return pytestconfig.option.service_logs_file
    return service_tmpdir / 'service.log'


@pytest.fixture(scope='session')
def _service_logfile_path(
        userver_register_logfile,
        service_logfile_path: typing.Optional[pathlib.Path],
) -> typing.Optional[pathlib.Path]:
    if not service_logfile_path:
        return None
    userver_register_logfile(
        service_logfile_path, title='userver/log', truncate=True,
    )
    return service_logfile_path


@pytest.fixture(scope='session')
def userver_register_logfile(_userver_logging_plugin: UserverLoggingPlugin):
    """
    Register logfile. Registered logfile is monitored in case of test failure
    and its contents is attached to pytest report.

    :param path: pathlib.Path corresponding to log file
    :param title: title to be used in pytest report
    :param truncate: file is truncated if True

    ```python
    def register_logfile(
            path: pathlib.Path, *, title: str, truncate: bool = False,
    ) -> None:
    ```
    """

    def do_truncate(path):
        with path.open('wb+') as fp:
            fp.truncate()

    def register_logfile(
            path: pathlib.Path, *, title: str, truncate: bool = False,
    ) -> None:
        if truncate:
            do_truncate(path)
        _userver_logging_plugin.register_logfile(path, title)
        return path

    return register_logfile


@pytest.fixture(scope='session')
def _userver_logging_plugin(pytestconfig) -> UserverLoggingPlugin:
    return pytestconfig.pluginmanager.get_plugin('userver_logging')


def pytest_configure(config):
    live_logs_enabled = bool(
        config.option.capture == 'no'
        and config.option.showcapture in ('all', 'log'),
    )

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


def pytest_report_header(config):
    headers = []
    if config.option.service_logs_file:
        headers.append(f'servicelogs: {config.option.service_logs_file}')
    return headers


def pytest_terminal_summary(terminalreporter, config) -> None:
    logfile = config.option.service_logs_file
    if logfile:
        terminalreporter.ensure_newline()
        terminalreporter.section('Service logs', sep='-', blue=True, bold=True)
        terminalreporter.write_sep('-', f'service log file: {logfile}')


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
