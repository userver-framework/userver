import asyncio

import pytest


pytest_plugins = ['pytest_userver.plugins.core']

USERVER_CONFIG_HOOKS = ['config_echo_url']


@pytest.fixture(scope='session')
def config_echo_url(mockserver_info):
    def _do_patch(config_yaml, config_vars):
        components = config_yaml['components_manager']['components']
        components['handler-echo-no-body']['echo-url'] = mockserver_info.url(
            '/test-service/echo-no-body',
        )

    return _do_patch


@pytest.fixture(name='jaeger_logs_path', scope='session')
def _jaeger_logs_path(tmp_path_factory):
    return str(tmp_path_factory.mktemp('opentracing_dir') / 'log.txt')


# Overriding the default testsuite behavior
@pytest.fixture(name='userver_config_logging', scope='session')
def _userver_config_logging(userver_config_logging, jaeger_logs_path):
    def _patch_config(config_yaml, config_vars):
        userver_config_logging(config_yaml, config_vars)
        logging = config_yaml['components_manager']['components']['logging']
        logging['loggers']['opentracing'] = {
            'file_path': jaeger_logs_path,
            'level': 'info',
            'flush_level': 'info',
            'overflow_behavior': 'block',
        }

    return _patch_config


@pytest.fixture(scope='function')
async def assert_ids_in_file(service_client, jaeger_logs_path):
    with open(jaeger_logs_path, 'w') as jaeger_file:
        jaeger_file.truncate(0)

    trace_id = ''

    # Checking the capture only after the capturing was stopped, to make sure
    # that the logs were flushed
    async def _check_the_files(check_trace_id: str):
        nonlocal trace_id
        assert not trace_id, 'Fixture assert_ids_in_file was invoked twice'
        trace_id = check_trace_id

    async with service_client.capture_logs() as capture:
        yield _check_the_files
        assert trace_id, 'Fixture assert_ids_in_file was not invoked'

    records = capture.select(trace_id=trace_id)
    assert len(records) >= 1, capture.select()

    retries = 100
    required_data = {
        f'\ttrace_id={trace_id}',
        'service_name=http-tracing-test',
        'duration=',
        'operation_name=external',
        'tags=[{"',
        'test-service/echo-no-body"',
        '"key":"http.url"',
        '"key":"http.status_code"',
        '"value":"200"',
        '}]',
    }

    for _ in range(retries):
        probable_lines = []
        with open(jaeger_logs_path, 'r') as jaeger_file:
            for line in reversed(jaeger_file.read().split('\n')):
                if trace_id in line:
                    probable_lines.append(line)

                if all(substr in line for substr in required_data):
                    return

        await asyncio.sleep(0.5)
    assert False, (
        f'Missing substrings {required_data} in opentracing file '
        f'for trace id {trace_id}. Lines:\n {probable_lines}'
    )
