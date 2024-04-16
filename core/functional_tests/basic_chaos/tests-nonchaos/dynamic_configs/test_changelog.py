import contextlib

import pytest
from pytest_userver.plugins import dynamic_config


class ConfigHelper:
    def __init__(self, *, cache_invalidation_state, defaults):
        self.cache_invalidation_state = cache_invalidation_state
        self.defaults = defaults
        self.changelog = dynamic_config._Changelog()

    @contextlib.contextmanager
    def new_config(self):
        config = dynamic_config.DynamicConfig(
            initial_values=self.defaults,
            config_cache_components=[],
            cache_invalidation_state=self.cache_invalidation_state,
            changelog=self.changelog,
        )
        with self.changelog.rollback(self.defaults):
            yield config

    def get_updated_since(self, config, timestamp):
        updates = self.changelog.get_updated_since(
            config.get_values_unsafe(), timestamp,
        )
        return updates.timestamp, updates


@pytest.fixture
def config_helper(cache_invalidation_state):
    def factory(*, defaults):
        return ConfigHelper(
            cache_invalidation_state=cache_invalidation_state,
            defaults=defaults,
        )

    return factory


def test_basic(config_helper):
    helper = config_helper(defaults={'FOO': 1, 'BAR': 1})

    timestamp = ''

    with helper.new_config() as config:
        timestamp, updates = helper.get_updated_since(config, timestamp)
        assert updates.values == {'FOO': 1, 'BAR': 1}

        timestamp, updates = helper.get_updated_since(config, timestamp)
        assert updates.values == {}
        config.set(FOO=2)

        timestamp, updates = helper.get_updated_since(config, timestamp)
        assert updates.values == {'FOO': 2}

    with helper.new_config() as config:
        config.set(BAR=2)

        timestamp, updates = helper.get_updated_since(config, timestamp)
        assert updates.values == {'FOO': 1, 'BAR': 2}

        timestamp, updates = helper.get_updated_since(config, timestamp)
        assert updates.values == {}

    with helper.new_config() as config:
        config.set(BAR=2)

        timestamp, updates = helper.get_updated_since(config, timestamp)
        assert updates.values == {}


def test_removal(config_helper):
    helper = config_helper(defaults={'FOO': 1})

    timestamp = ''

    with helper.new_config() as config:
        timestamp, updates = helper.get_updated_since(config, timestamp)
        assert updates.values == {'FOO': 1}
        assert updates.removed == []

        timestamp, updates = helper.get_updated_since(config, timestamp)
        assert updates.values == {}
        assert updates.removed == []
        config.remove('FOO')

        timestamp, updates = helper.get_updated_since(config, timestamp)
        assert updates.values == {}
        assert updates.removed == ['FOO']

    with helper.new_config() as config:
        timestamp, updates = helper.get_updated_since(config, timestamp)
        assert updates.values == {'FOO': 1}

        timestamp, updates = helper.get_updated_since(config, timestamp)
        assert updates.values == {}
        config.set(BAR=2)

        timestamp, updates = helper.get_updated_since(config, timestamp)
        assert updates.values == {'BAR': 2}

    with helper.new_config() as config:
        timestamp, updates = helper.get_updated_since(config, timestamp)
        assert updates.values == {}
        assert updates.removed == ['BAR']


def test_set_twice(config_helper):
    helper = config_helper(defaults={'FOO': 1})

    timestamp = ''

    with helper.new_config() as config:
        timestamp, updates = helper.get_updated_since(config, timestamp)
        assert updates.values == {'FOO': 1}
        assert updates.removed == []

    with helper.new_config() as config:
        config.set(FOO=2)
        timestamp, updates = helper.get_updated_since(config, timestamp)
        assert updates.values == {'FOO': 2}
        assert updates.removed == []

    with helper.new_config() as config:
        config.set(FOO=2)
        timestamp, updates = helper.get_updated_since(config, timestamp)
        assert updates.values == {}
        assert updates.removed == []

    with helper.new_config() as config:
        timestamp, updates = helper.get_updated_since(config, timestamp)
        assert updates.values == {'FOO': 1}
        assert updates.removed == []
