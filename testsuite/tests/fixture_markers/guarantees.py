"""Fixtures shared by the userver-specific marker contract tests."""

from dataclasses import dataclass

import pytest
from pytest_userver import config
from pytest_userver import fixture_markers
from pytest_userver import service


@dataclass(frozen=True)
class OrderInfo:
    value: str


@dataclass(frozen=True)
class OtherInfo:
    value: str


def ordered(value):
    def decorate(func):
        return fixture_markers.mark(func, OrderInfo(value))

    return decorate


def other(func):
    return fixture_markers.mark(func, OtherInfo('other-type'))


class LaterPlugin:
    """Defined before GuaranteesPlugin (lower stamps), registered after it."""

    @pytest.fixture(scope='session')
    @ordered('later-plugin-first')
    def z_later_plugin(self):
        return 'later-plugin-first'

    @pytest.fixture(scope='session')
    @ordered('later-plugin-last')
    def a_later_plugin(self):
        return 'later-plugin-last'


class GuaranteesPlugin:
    # Names deliberately disagree with definition order.
    @pytest.fixture(scope='session')
    @ordered('first')
    def z_first(self):
        return 'first'

    @pytest.fixture(scope='session')
    @ordered('original')
    def overridden(self):
        pytest.fail('Discovery must not execute the overridden fixture')

    @pytest.fixture(scope='session')
    @ordered('last')
    def a_last(self):
        return 'last'

    @pytest.fixture(scope='session')
    @config.patch
    def z_config_first(self):
        def patch(config_yaml, config_vars):
            config_yaml['calls'].append('first')

        return patch

    @pytest.fixture(scope='session')
    @config.patch
    def inherited_config_patch(self):
        pytest.fail('The unmarked config-patch override must win')

    @pytest.fixture(scope='session')
    @config.patch
    def a_config_last(self):
        def patch(config_yaml, config_vars):
            config_yaml['calls'].append('last')

        return patch

    @pytest.fixture
    @service.dependency
    def inherited_service_dependency(self):
        pytest.fail('The unmarked service-dependency override must win')

    @pytest.fixture(scope='session')
    @ordered('wide')
    def narrowed(self):
        return 'wide'
