import contextlib
import pathlib

import pytest

import sample_utils

THIS_DIR = pathlib.Path(__file__).resolve().parent


def pytest_addoption(parser) -> None:
    parser.addoption(
        '--build-dir',
        action='store',
        default=THIS_DIR / '..' / '..' / 'build',
        help='The path to userver build directory, the default is '
        '{USERVER_SOURCE_DIR}/build',
    )
    parser.addoption(
        '--service-wait',
        action='store_true',
        default=False,
        help='If true, the service must be launched manually '
        'using the provided command',
    )


@pytest.fixture(name='build_dir', scope='session')
def _build_dir(request) -> pathlib.Path:
    return pathlib.Path(request.config.getoption('--build-dir')).resolve()


@pytest.fixture(name='start_service')
def _start_service(build_dir: pathlib.Path, request, tmp_path_factory, capfd):
    service_wait = request.config.getoption('--service-wait')

    @contextlib.contextmanager
    def start_service(service_name: str, *, port: int, timeout: float = 10.0):
        service = sample_utils.start_service(
            service_name,
            port=port,
            timeout=timeout,
            build_dir=build_dir,
            tmp_path_factory=tmp_path_factory,
            service_wait=service_wait,
            capfd=capfd,
        )

        try:
            yield
        finally:
            if service:
                service.terminate()

    return start_service
