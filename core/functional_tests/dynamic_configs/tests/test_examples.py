import pytest

# /// [kill switch disabling using a pytest marker]
from pytest_userver.plugins import dynamic_config as dynamic_config_lib


@pytest.mark.config(
    USERVER_LOG_REQUEST=dynamic_config_lib.USE_STATIC_DEFAULT,
    USERVER_LOG_REQUEST_HEADERS=dynamic_config_lib.USE_STATIC_DEFAULT,
)
async def test_something(service_client):
    # Test implementation
    ...
    # /// [kill switch disabling using a pytest marker]


# /// [pytest marker basic usage]
@pytest.mark.config(
    USERVER_LOG_REQUEST=False,
    USERVER_LOG_REQUEST_HEADERS=True,
)
async def test_whatever(service_client):
    # Test implementation
    ...
    # /// [pytest marker basic usage]


# /// [pytest marker in a variable]
# Define config marker
MY_CONFIG_VALUES = pytest.mark.config(
    USERVER_LOG_REQUEST=False,
    USERVER_LOG_REQUEST_HEADERS=True,
)


# Apply to specific tests
@MY_CONFIG_VALUES
async def test_with_config(service_client):
    # Test implementation
    ...


async def test_without_config(service_client):
    # Uses directory/file configs or defaults
    ...
    # /// [pytest marker in a variable]


async def test_kill_switches(service_client, dynamic_config):
    # /// [dynamic_config usage with kill switches]
    # Enable kill switches and set their values
    dynamic_config.set(
        FIRST_KILL_SWITCH=1,
        SECOND_KILL_SWITCH=2,
    )
    await service_client.update_server_state()

    # Disable kill switches
    dynamic_config.switch_to_static_default(
        'FIRST_KILL_SWITCH',
        'SECOND_KILL_SWITCH',
    )
    await service_client.update_server_state()

    # Enable kill switches without changing their values
    dynamic_config.switch_to_dynamic_value(
        'FIRST_KILL_SWITCH',
        'SECOND_KILL_SWITCH',
    )
    await service_client.update_server_state()
    # /// [dynamic_config usage with kill switches]
