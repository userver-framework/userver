import pytest

try:
    from src.proto.grpc.health.v1 import health_pb2_grpc
except ImportError:
    from health.v1 import health_pb2_grpc

pytest_plugins = ['pytest_userver.plugins.grpc']


@pytest.fixture
def grpc_client(grpc_channel):
    return health_pb2_grpc.HealthStub(grpc_channel)


@pytest.fixture(name='userver_config_testsuite', scope='session')
def _userver_config_testsuite(userver_config_testsuite):
    def patch_config(config, config_vars) -> None:
        userver_config_testsuite(config, config_vars)
        # Restore the option after it's deleted by the base fixture.
        # Don't do this in your testsuite tests! For userver tests only.
        config['components_manager']['graceful_shutdown_interval'] = '3s'

    return patch_config


@pytest.fixture(scope='session')
def graceful_shutdown_headers() -> dict[str, list[str]]:
    return {'x-envoy-immediate-health-check-fail': ['true']}


@pytest.fixture(scope='session')
def dynamic_config_fallback_patch(graceful_shutdown_headers):
    return {'GRACEFUL_SHUTDOWN_HEADERS': {'enabled': True, 'headers': graceful_shutdown_headers}}
