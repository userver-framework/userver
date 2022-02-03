import pathlib
import socket
import subprocess
import time
from typing import Optional

SERVICE_HOST = 'localhost'

THIS_DIR = pathlib.Path(__file__).resolve().parent


def copy_service_configs(
        service_name: str, *, destination: pathlib.Path,
) -> None:
    path_suffixes = [
        'static_config.yaml',
        'dynamic_config_fallback.json',
        'config_vars.yaml',
        'secure_data.json',
    ]

    for path_suffix in path_suffixes:
        source_path = THIS_DIR / '..' / service_name / path_suffix
        if not source_path.is_file():
            continue

        conf = source_path.read_text()
        conf = conf.replace('/etc/' + service_name, str(destination))
        conf = conf.replace('/var/cache/' + service_name, str(destination))
        conf = conf.replace('/var/log/' + service_name, str(destination))
        conf = conf.replace('/var/run/' + service_name, str(destination))
        (destination / path_suffix).write_text(conf)


def _is_service_running(*, port: int, timeout: float):
    try:
        with socket.create_connection((SERVICE_HOST, port), timeout=timeout):
            return True
    except OSError:
        return False


def _wait_for_service(
        *,
        port: int,
        binary_path: pathlib.Path,
        runtime_dir: pathlib.Path,
        capfd,
) -> None:
    timeout = 1
    if _is_service_running(port=port, timeout=timeout):
        return

    with capfd.disabled():
        config_path = runtime_dir / 'static_config.yaml'
        print()
        print(
            'The service is not running yet. Please start it manually, '
            'e.g. using gdb:',
        )
        print(f'gdb --args {binary_path} -c {config_path}')

        while True:
            if _is_service_running(port=port, timeout=timeout):
                break
            time.sleep(1)
            print('.', end='', flush=True)
        print()


def _do_start_service(
        *,
        port: int,
        timeout: float,
        binary_path: pathlib.Path,
        runtime_dir: pathlib.Path,
) -> subprocess.Popen:
    config_path = runtime_dir / 'static_config.yaml'
    service = subprocess.Popen([binary_path, '--config', config_path])

    start_time = time.perf_counter()
    while True:
        time_passed = time.perf_counter() - start_time
        if time_passed >= timeout:
            service.terminate()
            raise TimeoutError(
                f'Waited too long for the port {port} on host '
                f'{SERVICE_HOST} to start accepting connections.',
            )

        if _is_service_running(port=port, timeout=timeout - time_passed):
            break
        time.sleep(0.1)

    return service


def start_service(
        service_name: str,
        *,
        port: int,
        timeout: float,
        build_dir: pathlib.Path,
        tmp_path_factory,
        capfd,
        service_wait: bool,
) -> Optional[subprocess.Popen]:
    temp_dir_name = tmp_path_factory.mktemp(service_name)
    print(f'Copying "{service_name}" configs into: {temp_dir_name}')
    copy_service_configs(service_name, destination=temp_dir_name)

    binary_name = 'userver-samples-' + service_name
    binary_path = build_dir / 'samples' / service_name / binary_name

    if service_wait:
        _wait_for_service(
            port=port,
            binary_path=binary_path,
            runtime_dir=temp_dir_name,
            capfd=capfd,
        )
        return None

    return _do_start_service(
        port=port,
        timeout=timeout,
        binary_path=binary_path,
        runtime_dir=temp_dir_name,
    )
