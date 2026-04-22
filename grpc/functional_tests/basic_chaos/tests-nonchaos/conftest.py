from typing import Any

import pytest

import samples.greeter_pb2_grpc as greeter_pb2_grpc

pytest_plugins = ['pytest_userver.plugins.grpc']


@pytest.fixture(scope='session')
def service_env():
    return {'CPU_LIMIT': '1c'}


@pytest.fixture(scope='session')
def congestion_control_fake_mode() -> bool:
    return False


@pytest.fixture(scope='session')
def dynamic_config_fallback_patch() -> dict[str, Any]:
    return {
        'USERVER_RPS_CCONTROL_ENABLED': True,
        'USERVER_RPS_CCONTROL': {
            'min-limit': 1,
            'up-rate-percent': 100,
            'down-rate-percent': 100,
            'overload-on-seconds': 1,
            'overload-off-seconds': 1,
            'no-limit-seconds': 1,
            'start-limit-factor': 0.01,
        },
    }


@pytest.fixture
def grpc_client(grpc_channel):
    return greeter_pb2_grpc.GreeterServiceStub(grpc_channel)
