import io
import pathlib
import sys
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

    def readlines(self):
        with self.path.open('rb') as fp:
            fp.seek(self.position)
            first_skipped = False
            for line in fp:
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


class UserverLoggingPlugin:
    log_file: typing.Optional[LogFile] = None

    def __init__(self, *, colorize_factory):
        self._colorize_factory = colorize_factory

    def register_log_file(self, path: pathlib.Path):
        self.log_file = LogFile(path)

    def pytest_runtest_setup(self, item):
        if self.log_file:
            self.log_file.update_position()

    @pytest.hookimpl(wrapper=True)
    def pytest_runtest_teardown(self, item):
        yield from self._userver_log_dump(item, 'teardown')

    def _userver_log_dump(self, item, when):
        try:
            yield
        except Exception:
            self._userver_report_attach(item, when)
            raise
        if item.utestsuite_report.failed:
            self._userver_report_attach(item, when)

    def _userver_report_attach(self, item, when):
        if not self.log_file:
            return
        report = io.StringIO()
        colorizer = self._colorize_factory()
        for line in self.log_file.readlines():
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
    userver_logging: UserverLoggingPlugin = pytestconfig.pluginmanager.get_plugin(
        'userver_logging'
    )

    path = pytestconfig.option.service_logs_file
    if path is None:
        path = service_tmpdir / 'service.log'
    userver_logging.register_log_file(path)
    return path


def pytest_configure(config):
    pretty_logs = config.option.service_logs_pretty
    colors_enabled = _should_enable_color(config)
    verbose = pretty_logs == 'verbose'

    def colorize_factory():
        if pretty_logs:
            colorizer = colorize.Colorizer(
                verbose=verbose,
                colors_enabled=colors_enabled,
            )
            def handle_line(line):
                return colorizer.colorize_line(line)
        else:
            def handle_line(line):
                return line
        return handle_line

    config.pluginmanager.register(
        UserverLoggingPlugin(colorize_factory=colorize_factory),
        'userver_logging'
    )



def _should_enable_color(pytestconfig) -> bool:
    option = getattr(pytestconfig.option, 'color', 'no')
    if option == 'yes':
        return True
    if option == 'auto':
        return sys.stderr.isatty()
    return False
