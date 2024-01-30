import contextlib

from pytest_userver.plugins import dynamic_config


def test_basic(cache_invalidation_state):
    defaults = {'FOO': 1, 'BAR': 1}
    changelog = dynamic_config._Changelog()

    timestamp = ''

    @contextlib.contextmanager
    def new_config():
        config = dynamic_config.DynamicConfig(
            initial_values=defaults,
            config_cache_components=[],
            cache_invalidation_state=cache_invalidation_state,
            changelog=changelog,
        )
        with changelog.rollback(defaults):
            yield config

    def get_updated_since():
        updates = changelog.get_updated_since(
            config.get_values_unsafe(), timestamp,
        )
        return updates.timestamp, updates

    with new_config() as config:
        timestamp, updates = get_updated_since()
        assert updates.values == {'FOO': 1, 'BAR': 1}

        timestamp, updates = get_updated_since()
        assert updates.values == {}
        config.set(FOO=2)

        timestamp, updates = get_updated_since()
        assert updates.values == {'FOO': 2}

    with new_config() as config:
        config.set(BAR=2)

        timestamp, updates = get_updated_since()
        assert updates.values == {'FOO': 1, 'BAR': 2}

        timestamp, updates = get_updated_since()
        assert updates.values == {}

    with new_config() as config:
        config.set(BAR=2)

        timestamp, updates = get_updated_since()
        assert updates.values == {}


def test_removal(cache_invalidation_state):
    defaults = {'FOO': 1}
    changelog = dynamic_config._Changelog()

    timestamp = ''

    @contextlib.contextmanager
    def new_config():
        config = dynamic_config.DynamicConfig(
            initial_values=defaults,
            config_cache_components=[],
            cache_invalidation_state=cache_invalidation_state,
            changelog=changelog,
        )
        with changelog.rollback(defaults):
            yield config

    def get_updated_since():
        updates = changelog.get_updated_since(
            config.get_values_unsafe(), timestamp,
        )
        return updates.timestamp, updates

    with new_config() as config:
        timestamp, updates = get_updated_since()
        assert updates.values == {'FOO': 1}
        assert updates.removed == []

        timestamp, updates = get_updated_since()
        assert updates.values == {}
        assert updates.removed == []
        config.remove('FOO')

        timestamp, updates = get_updated_since()
        assert updates.values == {}
        assert updates.removed == ['FOO']

    with new_config() as config:
        timestamp, updates = get_updated_since()
        assert updates.values == {'FOO': 1}

        timestamp, updates = get_updated_since()
        assert updates.values == {}
        config.set(BAR=2)

        timestamp, updates = get_updated_since()
        assert updates.values == {'BAR': 2}

    with new_config() as config:
        timestamp, updates = get_updated_since()
        assert updates.values == {}
        assert updates.removed == ['BAR']


def test_cache_config(userver_cache_config):
    assert (
        'dynamic-config-client-updater'
        in userver_cache_config.incremental_caches
    )
