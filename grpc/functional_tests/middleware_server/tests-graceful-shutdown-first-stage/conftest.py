from typing import Any

import pytest

import samples.greeter_pb2_grpc as greeter_services

pytest_plugins = ['pytest_userver.plugins.grpc']


@pytest.fixture(scope='session')
def dynamic_config_fallback_patch() -> dict[str, Any]:
    return {'USERVER_RPS_CCONTROL_ENABLED': True}


@pytest.fixture(scope='session')
def congestion_control_fake_mode() -> bool:
    return False


@pytest.fixture(scope='session')
def service_env():
    return {'CPU_LIMIT': '1c'}


@pytest.fixture
def grpc_client(grpc_channel):
    return greeter_services.GreeterServiceStub(grpc_channel)


# Overriding a userver fixture
@pytest.fixture(name='userver_config_testsuite', scope='session')
def _userver_config_testsuite(userver_config_testsuite):
    def patch_config(config, config_vars) -> None:
        userver_config_testsuite(config, config_vars)
        # Restore the option after it's deleted by the base fixture.
        # Don't do this in your testsuite tests! For userver tests only.
        config['components_manager']['graceful_shutdown_continue_accepting_requests_interval'] = '3s'
        config['components_manager']['graceful_shutdown_pending_requests_completion_interval'] = '0s'

    return patch_config
