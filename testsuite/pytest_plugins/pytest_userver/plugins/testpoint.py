"""
Testpoints support for the server.
"""

# pylint: disable=redefined-outer-name
import typing

import pytest

USERVER_CONFIG_HOOKS = ['userver_config_testpoint']


class BaseError(Exception):
    pass


class UnregisteredTestpointError(BaseError):
    pass


class TestpointControl:
    def __init__(self):
        self.enabled_testpoints: typing.FrozenSet[str] = frozenset()


DISABLED_ERROR = """Access to {opname!r} on unregistered testpoint {name}

Use `await service_client.update_server_state()` to explicitly sync testpoints
state.
"""


@pytest.fixture
def testpoint_control():
    return TestpointControl()


@pytest.fixture
def testpoint_checker_factory(testpoint_control):
    def create_checker(name):
        def checker(opname):
            if name not in testpoint_control.enabled_testpoints:
                raise UnregisteredTestpointError(
                    DISABLED_ERROR.format(opname=opname, name=name),
                )

        return checker

    return create_checker


@pytest.fixture(scope='session')
def userver_config_testpoint(mockserver_info):
    """
    Returns a function that adjusts the static configuration file for
    the testsuite.
    Sets the `tests-control.skip-unregistered-testpoints` to `True`.

    @ingroup userver_testsuite_fixtures
    """

    def _patch_config(config_yaml, config_vars):
        components = config_yaml['components_manager']['components']
        tests_control = components.get('tests-control')
        if tests_control:
            tests_control['skip-unregistered-testpoints'] = True

    return _patch_config
