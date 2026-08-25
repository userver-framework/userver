import pytest

pytest_plugins = ['pytest_userver.plugins.core']


@pytest.fixture(scope='session')
def service_env():
    return {
        'MALLOC_CONF': 'prof:true,prof_active:true,prof_accum:true,lg_prof_sample:14',
    }
