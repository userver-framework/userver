"""Exercise the real userver consumer fixtures without starting a service."""

import pytest
from pytest_userver.plugins import config as config_plugin
from pytest_userver.plugins import service as service_plugin

_service_config_hooked = config_plugin._service_config_hooked
_mark_service_deps = service_plugin._mark_service_deps


@pytest.fixture(scope='session')
def _original_service_config():
    return config_plugin._UserverConfig(config_yaml={'calls': []}, config_vars={})


@pytest.fixture(scope='session')
def daemon_scoped_mark():
    return {}


@pytest.fixture(scope='session')
def inherited_config_patch():
    def patch(config_yaml, config_vars):
        config_yaml['calls'].append('override')
        config_vars['source'] = 'override'

    return patch


@pytest.fixture
def dependency_calls():
    return []


@pytest.fixture
def inherited_service_dependency(dependency_calls):
    dependency_calls.append('override')


def test_config_consumer_applies_unmarked_override_in_original_position(
    _service_config_hooked, _original_service_config
):
    assert _service_config_hooked.config_yaml == {'calls': ['first', 'override', 'last']}
    assert _service_config_hooked.config_vars == {'source': 'override'}
    assert _original_service_config.config_yaml == {'calls': []}
    assert _original_service_config.config_vars == {}


def test_service_consumer_executes_unmarked_dependency_override(_mark_service_deps, dependency_calls):
    assert dependency_calls == ['override']
