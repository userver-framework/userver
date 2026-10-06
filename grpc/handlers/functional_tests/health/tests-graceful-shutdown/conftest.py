import pytest

try:
    from grpc.health.v1 import health_pb2_grpc
except ImportError:
    from health.v1 import health_pb2_grpc

pytest_plugins = ['pytest_userver.plugins.grpc']

# During the first graceful shutdown stage the service reports NOT_SERVING, but still serves
# requests. Afterwards the gRPC server stops accepting new calls, so everything the test checks
# must happen within this interval.
FIRST_STAGE_SECONDS = 3


@pytest.fixture
def grpc_client(grpc_channel):
    return health_pb2_grpc.HealthStub(grpc_channel)


@pytest.fixture(name='userver_config_testsuite', scope='session')
def _userver_config_testsuite(userver_config_testsuite):
    def patch_config(config, config_vars) -> None:
        userver_config_testsuite(config, config_vars)
        # Restore the option after it's deleted by the base fixture.
        # Don't do this in your testsuite tests! For userver tests only.
        components_manager = config['components_manager']
        components_manager['graceful_shutdown_continue_accepting_requests_interval'] = f'{FIRST_STAGE_SECONDS}s'
        components_manager['graceful_shutdown_pending_requests_completion_interval'] = '6s'

    return patch_config


@pytest.fixture(scope='session')
def graceful_shutdown_first_stage_seconds() -> int:
    return FIRST_STAGE_SECONDS


@pytest.fixture(scope='session')
def graceful_shutdown_headers() -> dict[str, list[str]]:
    return {'x-envoy-immediate-health-check-fail': ['true']}


@pytest.fixture(scope='session')
def dynamic_config_fallback_patch(graceful_shutdown_headers):
    return {'GRACEFUL_SHUTDOWN_HEADERS': {'enabled': True, 'headers': graceful_shutdown_headers}}
