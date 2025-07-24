import pytest
from pytest_userver.plugins import dynamic_config as dynamic_config_lib

CONFIG_KEY = 'TEST_CONFIG_KEY'

NEW_VALUE = 'new_value'

KILL_SWITCH_DISABLED_MARK = pytest.mark.config(**{CONFIG_KEY: dynamic_config_lib.USE_STATIC_DEFAULT})
