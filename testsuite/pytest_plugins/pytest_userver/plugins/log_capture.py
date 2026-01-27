"""
Capture and work with logs.
"""

# pylint: disable=redefined-outer-name
import logging

import pytest

from testsuite import logcapture
from testsuite.logcapture import __tracebackhide__  # noqa

from ..utils import tskv

USERVER_CONFIG_HOOKS = ['_userver_config_logs_capture']
DEFAULT_PORT = 2211

logger = logging.getLogger(__name__)


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
async def userver_log_capture(pytestconfig, userver_log_level):
    host = pytestconfig.option.logs_capture_host
    port = pytestconfig.option.logs_capture_port
    if pytestconfig.option.service_wait or pytestconfig.option.service_disable:
        port = port or DEFAULT_PORT

    server = logcapture.CaptureServer(
        log_level=logcapture.LogLevel.from_string(userver_log_level),
        parse_line=_tskv_parse_line,
    )
    async with server.start(host=host, port=port):
        yield server


@pytest.fixture(scope='session')
def _userver_config_logs_capture(userver_log_capture):
    socknames = userver_log_capture.getsocknames()
    assert socknames
    sockname = socknames[0]

    def patch_config(config, _config_vars) -> None:
        logging_config = config['components_manager']['components']['logging']
        default_logger = logging_config['loggers']['default']
        # Other formats are not yet supported by log-capture.
        default_logger['format'] = 'tskv'
        default_logger['testsuite-capture'] = {
            'host': sockname[0],
            'port': sockname[1],
        }

    return patch_config


def _tskv_parse_line(rawline: bytes):
    line = rawline.decode(encoding='utf-8', errors='replace')
    return tskv.parse_line(line)
