import socket

import pytest


async def test_distlock(service_client, testpoint):
    @testpoint('distlock-worker')
    def distlock_tp(data):
        pass

    await service_client.run_distlock_task('distlock-pg')
    assert distlock_tp.times_called == 1


@pytest.fixture(scope='session')
def distlock_host_policy_config(request):
    def patch_config(config_yaml, _config_vars):
        components = config_yaml['components_manager']['components']
        defaults = components['dynamic-config'].setdefault('defaults', {})
        defaults['POSTGRES_DISTLOCK_SETTINGS'] = {
            'distlock-pg': {
                'acquire_interval': 10,
                'fallback_heartbeat_checks': 1,
                **request.param,
            },
        }

    return patch_config


@pytest.mark.uservice_oneshot(config_hooks=['distlock_host_policy_config'])
@pytest.mark.parametrize(
    'distlock_host_policy_config, expected_runs',
    [
        pytest.param(
            {'run_on_hosts': []},
            0,
            id='empty-preferred-disables-all',
        ),
        pytest.param(
            {'run_on_hosts': ['unavailable-preferred-host']},
            1,
            id='nonpreferred-host-automatically-runs',
        ),
        pytest.param(
            {'run_on_hosts': [socket.gethostname()]},
            1,
            id='preferred-host-runs',
        ),
        pytest.param(
            {},
            1,
            id='omitted-preferred-remains-unrestricted',
        ),
    ],
    indirect=['distlock_host_policy_config'],
)
async def test_distlock_host_policy(service_client, testpoint, distlock_host_policy_config, expected_runs):
    @testpoint('distlock-worker')
    def distlock_tp(data):
        pass

    await service_client.run_distlock_task('distlock-pg')
    assert distlock_tp.times_called == expected_runs
