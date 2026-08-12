import pytest

import samples.greeter_pb2_grpc as greeter_services

pytest_plugins = ['pytest_userver.plugins.grpc']

USERVER_CONFIG_HOOKS = ['config_echo_url']


@pytest.fixture(scope='session')
def config_echo_url(mockserver_info):
    def _do_patch(config_yaml, config_vars):
        components = config_yaml['components_manager']['components']
        components['greeter-service']['echo-url'] = mockserver_info.url(
            '/test-service/echo-no-body',
        )

    return _do_patch


@pytest.fixture
def grpc_client(grpc_channel):
    return greeter_services.GreeterServiceStub(grpc_channel)


@pytest.fixture(scope='session')
def disable_otel_trace_sampling():
    def _do_patch(config_yaml, _config_vars):
        config_yaml['components_manager']['components']['tracing-manager-locator']['otel-trace-sampling-enabled'] = (
            False
        )

    return _do_patch
