"""
Pytest plugin that starts a userver service through its `start-*` cmake target.

Unlike the standard service fixtures (which spawn the service binary directly),
this plugin overrides the service-start fixture so that the service is launched
via a parallel build of a `start-*` cmake target. That target runs the testsuite
runner in service-runner-mode.

The plugin adds three required pytest options:

* ``--cmake-command`` -- path to the cmake executable used to build the target;
* ``--cmake-build-dir`` -- the project build directory (``cmake --build`` arg);
* ``--service-start-target`` -- the name of the ``start-*`` cmake target.

Load it from a service's ``start_testsuite`` conftest via::

    pytest_plugins = [
        'pytest_userver.plugins.core',
        'start_target.pytest_plugin',
    ]

For this import to work, ``scripts/tests`` must be on the python path; the
service CMakeLists passes it through ``userver_testsuite_add_simple(PYTHONPATH
...)``.
"""

import logging
import os
import pathlib
import subprocess

import pytest
from pytest_userver import client

logger = logging.getLogger(__name__)


def pytest_addoption(parser) -> None:
    group = parser.getgroup('userver-start-testsuite')
    group.addoption(
        '--cmake-command',
        type=pathlib.Path,
        required=True,
        help='Path to the cmake executable used to build the start-* target.',
    )
    group.addoption(
        '--cmake-build-dir',
        type=pathlib.Path,
        required=True,
        help='Path to the project build directory (`cmake --build` argument).',
    )
    group.addoption(
        '--service-start-target',
        type=str,
        required=True,
        help='Name of the `start-*` cmake target that launches the service.',
    )


def _forward_signals_to_process_group(process: subprocess.Popen) -> None:
    """
    Make the daemon forward shutdown signals to its whole process group.

    The service is started as `cmake --build ... --target start-*`, which spawns
    a tree of processes (cmake -> build tool -> testsuite runner -> service).
    Signaling only the direct child would leave the actual service running, so
    we override the process' `send_signal` to deliver the signal to the entire
    process group instead. The daemon is spawned with `start_new_session=True`
    (see `subprocess_options` below), which makes it a process-group leader, so
    `os.getpgid(process.pid)` covers the whole tree.
    """
    original_send_signal = process.send_signal

    def send_signal(sig: int) -> None:
        try:
            os.killpg(os.getpgid(process.pid), sig)
        except (ProcessLookupError, PermissionError):
            original_send_signal(sig)

    process.send_signal = send_signal


@pytest.fixture(scope='session')
def service_env(service_port) -> dict:
    # Force the inner service-runner-mode session to bind exactly the port that
    # the outer session uses, via the userver `--service-port` pytest option.
    # Otherwise the inner runner keeps the static-config port only while it is
    # free and silently randomizes it on conflict, diverging from the outer
    # session's `service_port`. Pinning the inner runner to `service_port`
    # guarantees the two always agree, whatever value the outer session picked.
    # The option is delivered through PYTEST_ADDOPTS because the `start-*` cmake
    # target invokes the runner with a fixed command line.
    addopts = os.environ.get('PYTEST_ADDOPTS', '')
    forced_port = f'--service-port={service_port}'
    return {'PYTEST_ADDOPTS': f'{addopts} {forced_port}'.strip()}


@pytest.fixture(scope='session')
def _testsuite_client_config(service_config) -> client.TestsuiteClientConfig:
    # The service's `tests-control` testpoints talk to the mockserver of the
    # inner service-runner-mode session, not to this outer session. Force the
    # plain HTTP client (no tests-control handshake) so that requests go
    # straight to the running service.
    return client.TestsuiteClientConfig()


@pytest.fixture(scope='session')
async def service_daemon_scope(
    pytestconfig,
    create_daemon_scope,
    service_env,
    service_health_check,
):
    """
    Starts the service via the `start-*` cmake target instead of spawning the
    service binary directly.
    """
    cmake = str(pytestconfig.option.cmake_command)
    build_dir = str(pytestconfig.option.cmake_build_dir)
    start_target = pytestconfig.option.service_start_target

    logger.info(
        'Starting the service via "%s --build %s --target %s"',
        cmake,
        build_dir,
        start_target,
    )

    async with create_daemon_scope(
        args=[cmake, '--build', build_dir, '--target', start_target],
        health_check=service_health_check,
        env=service_env,
        # start_new_session=True puts the spawned `cmake --build` tree into its
        # own session/process group so that `setup_service` below can terminate
        # all of it at once on teardown.
        subprocess_options={'start_new_session': True},
        setup_service=_forward_signals_to_process_group,
    ) as scope:
        yield scope
