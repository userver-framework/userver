import dataclasses
import datetime
from typing import Iterable
from typing import Optional

import pytest
from pytest_userver.plugins import caches

pytest_plugins = ['pytest_userver.plugins.core']

CONFIG_KEY = 'TEST_CONFIG_KEY'


@dataclasses.dataclass(frozen=True)
class DynamicConfigState:
    value: str
    is_enabled: bool


class MockKillSwitchStorage:
    def __init__(
        self,
        is_enabled: bool,
        cache_invalidation_state: caches.InvalidationState,
        config_cache_components: Iterable[str],
    ) -> None:
        self._state = DynamicConfigState(
            value='initial_value',
            is_enabled=is_enabled,
        )
        self._updated_at = datetime.datetime.fromtimestamp(
            0,
            datetime.timezone.utc,
        )
        self._cache_invalidation_state = cache_invalidation_state
        self._config_cache_components = config_cache_components

    @property
    def config_value(self) -> str:
        return self._state.value

    @property
    def is_enabled(self) -> bool:
        return self._state.is_enabled

    @property
    def state(self) -> DynamicConfigState:
        return self._state

    @property
    def updated_at(self) -> datetime.datetime:
        return self._updated_at

    @config_value.setter
    def config_value(self, config_value: str) -> None:
        self._state = DynamicConfigState(
            value=config_value,
            is_enabled=self.is_enabled,
        )
        self._sync_with_service()

    @is_enabled.setter
    def is_enabled(self, is_enabled: bool) -> None:
        self._state = DynamicConfigState(
            value=self.config_value,
            is_enabled=is_enabled,
        )
        self._sync_with_service()

    def has_updates(self, client_updated_at: Optional[str]) -> bool:
        return client_updated_at is None or self._updated_at > datetime.datetime.strptime(
            client_updated_at,
            '%Y-%m-%dT%H:%M:%SZ',
        ).replace(tzinfo=datetime.timezone.utc)

    def _sync_with_service(self) -> None:
        self._updated_at += datetime.timedelta(seconds=1)
        self._cache_invalidation_state.invalidate(
            self._config_cache_components,
        )


@pytest.fixture
def mock_kill_switch_storage(
    initial_kill_switch_flag,
    cache_invalidation_state,
    dynconf_cache_names,
) -> MockKillSwitchStorage:
    return MockKillSwitchStorage(
        is_enabled=initial_kill_switch_flag,
        config_cache_components=dynconf_cache_names,
        cache_invalidation_state=cache_invalidation_state,
    )


def convert_to_timestring(timestamp: datetime.datetime) -> str:
    return timestamp.strftime('%Y-%m-%dT%H:%M:%SZ')


# Overrides mock_configs_service fixture in pytest_userver.plugins.dynamic_config
@pytest.fixture
def mock_configs_service(mockserver, mock_kill_switch_storage) -> None:
    @mockserver.json_handler('/configs-service/configs/values')
    def _mock_configs(request):
        client_updated_at = request.json.get('updated_since', None)
        server_updated_at = convert_to_timestring(
            mock_kill_switch_storage.updated_at,
        )

        if not mock_kill_switch_storage.has_updates(client_updated_at):
            return {'configs': {}, 'updated_at': server_updated_at}

        update = {
            'configs': {CONFIG_KEY: mock_kill_switch_storage.config_value},
            'updated_at': server_updated_at,
        }
        if not mock_kill_switch_storage.is_enabled:
            update['kill_switches_disabled'] = [CONFIG_KEY]
        return update


# overrides _userver_dynconfig_cache_control fixture in pytest_userver.plugins.dynamic_config
@pytest.fixture
def _userver_dynconfig_cache_control(mock_kill_switch_storage):
    def cache_control(_request, _state):
        return mock_kill_switch_storage.state

    return cache_control


@pytest.fixture
def config_update_testpoint(testpoint):
    @testpoint('config-update')
    def config_update(_data):
        pass

    return config_update


@pytest.fixture(name='clean_cache_and_flush_testpoint')
async def _clean_cache_and_flush_testpoint(
    config_update_testpoint,
    service_client,
):
    await service_client.update_server_state()
    config_update_testpoint.flush()


@pytest.fixture
async def check_fresh_update(
    service_client,
    config_update_testpoint,
    mock_kill_switch_storage,
):
    async def check():
        await service_client.update_server_state()
        assert await config_update_testpoint.wait_call() == {
            '_data': mock_kill_switch_storage.config_value,
        }

    return check


@pytest.fixture
async def check_no_update(service_client, config_update_testpoint):
    async def check():
        await service_client.update_server_state()
        assert config_update_testpoint.times_called == 0

    return check


@pytest.fixture
async def check_reset_to_default(
    service_client,
    config_update_testpoint,
    mock_kill_switch_storage,
):
    async def check():
        await service_client.update_server_state()

        default_config = await service_client.get_dynamic_config_defaults()
        static_default = default_config[CONFIG_KEY]
        assert await config_update_testpoint.wait_call() == {
            '_data': static_default,
        }

    return check
