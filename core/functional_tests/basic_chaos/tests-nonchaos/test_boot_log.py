import json

import pytest

from testsuite.daemons import spawn


def default_stderr_handler(line: str):
    pass


@pytest.fixture(scope='session')
def boot_log_path(service_tmpdir):
    return str(service_tmpdir / 'boot.log')


@pytest.fixture(scope='session')
def enable_boot_logger(boot_log_path):
    def patch_config(config_yaml, _config_vars) -> None:
        config_yaml['components_manager']['boot-log-path'] = boot_log_path

    return patch_config


@pytest.fixture(scope='session')
def break_logger_config(boot_log_path):
    def patch_config(config_yaml, _config_vars) -> None:
        config_yaml['components_manager']['components']['logging']['loggers']['default']['file_path'] = '/'

    return patch_config


@pytest.fixture
def assert_service_boot_fails(
    service_binary,
    service_config_path_temp,
    service_env,
    create_daemon_scope,
    ensure_daemon_started,
    service_http_ping_url,
):
    async def func(stderr_handler=default_stderr_handler):
        async with create_daemon_scope(
            args=[
                str(service_binary),
                '--config',
                str(service_config_path_temp),
            ],
            env=service_env,
            ping_url=service_http_ping_url,
            stderr_handler=stderr_handler,
        ) as scope:
            with pytest.raises((spawn.HealthCheckError, spawn.ExitCodeError)):
                await ensure_daemon_started(scope)

    return func


def read_boot_log(path: str) -> list[dict[str, str | int]]:
    with open(path) as ifile:
        content = ifile.read()

    logs = []
    for line in content.splitlines():
        line = line.strip()
        if not line:
            continue

        try:
            logs.append(json.loads(line))
        except BaseException:
            pytest.fail('Invalid JSON string in bool log: {line}')
    return logs


@pytest.mark.uservice_oneshot(config_hooks=['enable_boot_logger'])
async def test_boot_ok(service_client, boot_log_path):
    logs = read_boot_log(boot_log_path)

    assert logs[0]['event_type'] == 'component_system_is_starting'

    components = list(
        map(lambda x: x['component_name'], filter(lambda x: x['event_type'] == 'component_is_starting', logs))
    )

    checked_components = {
        'dns-client',
        'http-client',
        'logging',
        'logging-configurator',
        'manager-controller',
        'os-signal-processor',
        'server',
        'statistics-storage',
        'tests-control',
        'testsuite-support',
    }
    assert checked_components.issubset(components)

    assert logs[-1]['event_type'] == 'component_system_is_started'


@pytest.mark.uservice_oneshot(config_hooks=['enable_boot_logger', 'break_logger_config'])
async def test_boot_fail(
    assert_service_boot_fails,
    boot_log_path,
):
    await assert_service_boot_fails()

    logs = read_boot_log(boot_log_path)
    logging_events = list(filter(lambda x: x.get('component_name') == 'logging', logs))

    # 'logging' component is at least created
    assert logging_events[0]['event_type'] == 'component_is_starting'

    # Boot failure may lead to crash, so the last log item event_type is anything, but 'component_system_is_started'
    assert logs[-1]['event_type'] != 'component_system_is_started'


@pytest.fixture(scope='session')
def unix_socket_logger_with_truncate():
    def patch_config(config_yaml, _config_vars) -> None:
        config_yaml['components_manager']['components']['logging']['loggers']['unix'] = {
            'file_path': 'unix:/path',
            'truncate-on-start': True,
        }

    return patch_config


@pytest.mark.uservice_oneshot(config_hooks=['unix_socket_logger_with_truncate'])
async def test_unix_socket_with_truncate(
    assert_service_boot_fails,
    _service_logfile_path,
):
    stderr = bytes()

    def handler(line: bytes):
        nonlocal stderr
        stderr += line

    await assert_service_boot_fails(stderr_handler=handler)

    expected_msg = (
        "Cannot start component logging: truncate-on-start cannot be combined with unix socket path for logger 'unix'"
    )
    assert expected_msg in stderr.decode('utf-8')
