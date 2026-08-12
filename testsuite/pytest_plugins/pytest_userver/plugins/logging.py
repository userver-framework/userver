from __future__ import annotations

import logging
import pathlib

import pytest

from pytest_userver.utils import colorize

logger = logging.getLogger(__name__)


@pytest.fixture(scope='session')
def service_logfile_path(
    pytestconfig,
    service_tmpdir: pathlib.Path,
) -> pathlib.Path | None:
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
    service_logfile_path: pathlib.Path | None,
) -> pathlib.Path | None:
    if not service_logfile_path:
        return None
    userver_register_logfile(
        service_logfile_path,
        title='userver/log',
        truncate=True,
    )
    return service_logfile_path


@pytest.fixture(scope='session')
def userver_register_logfile(servicelogs_register_logfile, _userver_log_formatter_factory):
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
        path: pathlib.Path,
        *,
        title: str,
        truncate: bool = False,
    ) -> None:
        if truncate:
            do_truncate(path)
        servicelogs_register_logfile(
            path,
            title=title,
            formatter_factory=_userver_log_formatter_factory,
        )
        return path

    return register_logfile


@pytest.fixture(scope='session')
def _userver_log_formatter_factory(pytestconfig, testsuite_colors_enabled):
    def colorizer_factory():
        verbose = pytestconfig.option.service_logs_pretty == 'verbose'
        colorizer = colorize.Colorizer(
            verbose=verbose,
            colors_enabled=testsuite_colors_enabled,
        )

        def format_line(rawline):
            line = rawline.decode(encoding='utf-8', errors='backslashreplace')
            if not line.startswith('tskv\t'):
                return None
            return colorizer.colorize_line(line)

        return format_line

    def default_factory():
        def format_line(rawline):
            return rawline.decode(encoding='utf-8', errors='backslashreplace')

        return format_line

    if pytestconfig.option.service_logs_pretty:
        return colorizer_factory
    return default_factory
