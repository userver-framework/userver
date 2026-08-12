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
