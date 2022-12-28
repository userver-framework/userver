"""
Start the service in testsuite.
"""

import pathlib
import sys
import traceback
import typing

import pytest

from testsuite.logging import logger
from testsuite.utils import url_util

from ..utils import colorize


class ColorLogger(logger.Logger):
    def __init__(
            self, *, writer: logger.LineLogger, verbose, colors_enabled,
    ) -> None:
        super().__init__(writer)
        self._colorizer = colorize.Colorizer(
            verbose=verbose, colors_enabled=colors_enabled,
        )

    def log_service_line(self, line) -> None:
        line = self._colorizer.colorize_line(line)
        if line:
            self.writeline(line)

    def log_entry(self, entry: dict) -> None:
        line = self._colorizer.colorize_row(entry)
        if line:
            self.writeline(line)


def pytest_addoption(parser) -> None:
    group = parser.getgroup('userver')
    group.addoption(
        '--service-logs-file',
        type=pathlib.Path,
        help='Write service output to specified file',
    )
    group.addoption(
        '--service-logs-pretty',
        action='store_true',
        help='Enable pretty print and colorize service logs',
    )
    group.addoption(
        '--service-logs-pretty-verbose',
        dest='service_logs_pretty',
        action='store_const',
        const='verbose',
        help='Enable pretty print and colorize service logs in verbose mode',
    )
    group.addoption(
        '--service-logs-pretty-disable',
        action='store_false',
        dest='service_logs_pretty',
        help='Disable pretty print and colorize service logs',
    )


def pytest_override_testsuite_logger(
        config, line_logger: logger.LineLogger, colors_enabled: bool,
) -> typing.Optional[logger.Logger]:
    pretty_logs = config.option.service_logs_pretty
    if not pretty_logs:
        return None
    return ColorLogger(
        writer=line_logger,
        verbose=pretty_logs == 'verbose',
        colors_enabled=colors_enabled,
    )


@pytest.fixture(scope='session')
def service_env():
    """
    Override this to pass extra environment variables to the service.

    @snippet samples/redis_service/tests/conftest.py service_env
    @ingroup userver_testsuite_fixtures
    """
    return None


@pytest.fixture(scope='session')
async def service_daemon(
        pytestconfig,
        create_daemon_scope,
        service_env,
        service_baseurl,
        service_config_path_temp,
        service_config_yaml,
        service_binary,
        service_port,
        create_port_health_checker,
        testsuite_logger,
        _userver_log_handler,
):
    components = service_config_yaml['components_manager']['components']

    ping_url = None
    health_check = None

    if 'handler-ping' in components:
        ping_url = url_util.join(
            service_baseurl, components['handler-ping']['path'],
        )
    else:
        health_check = create_port_health_checker(
            hostname='localhost', port=service_port,
        )

    async with create_daemon_scope(
            args=[
                str(service_binary),
                '--config',
                str(service_config_path_temp),
            ],
            ping_url=ping_url,
            health_check=health_check,
            env=service_env,
            stderr_handler=_userver_log_handler,
    ) as scope:
        yield scope


@pytest.fixture(scope='session')
def _userver_log_handler(pytestconfig, testsuite_logger, _uservice_logfile):
    service_logs_pretty = pytestconfig.option.service_logs_pretty
    if not service_logs_pretty and not bool(_uservice_logfile):
        return None

    if service_logs_pretty:
        logger_plugin = pytestconfig.pluginmanager.getplugin(
            'testsuite_logger',
        )
        logger_plugin.enable_logs_suspension()

    def log_handler(line_binary):
        if _uservice_logfile:
            _uservice_logfile.write(line_binary)
        try:
            line = line_binary.decode('utf-8').rstrip('\r\n')
            testsuite_logger.log_service_line(line)
        # flake8: noqa
        except:
            traceback.print_exc(file=sys.stderr)

    return log_handler


@pytest.fixture(scope='session')
def _uservice_logfile(pytestconfig):
    path = pytestconfig.option.service_logs_file
    if not path:
        yield None
    else:
        with path.open('wb') as fp:
            yield fp
