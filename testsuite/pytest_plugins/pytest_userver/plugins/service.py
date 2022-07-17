import pytest
from testsuite.utils import url_util


@pytest.fixture(scope='session')
def service_env():
    """
    Override this to pass extra environment variables to the service.
    """
    return None


@pytest.fixture(scope='session')
async def service_daemon(
        pytestconfig,
        create_daemon_scope,
        service_env,
        service_baseurl,
        service_config_path_temp,
        service_config_yaml,
        service_binary,
        service_port,
        create_port_health_checker,
):
    components = service_config_yaml['components_manager']['components']

    ping_url = None
    health_check = None

    if 'handler-ping' in components:
        ping_url = url_util.join(
            service_baseurl, components['handler-ping']['path'],
        )
    else:
        health_check = create_port_health_checker(
            hostname='localhost', port=service_port,
        )

    async with create_daemon_scope(
            args=[
                str(service_binary),
                '--config',
                str(service_config_path_temp),
            ],
            ping_url=ping_url,
            health_check=health_check,
            env=service_env,
    ) as scope:
        yield scope
