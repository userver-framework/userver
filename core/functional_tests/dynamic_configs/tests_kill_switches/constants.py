import pytest
from pytest_userver import dynconf

CONFIG_KEY = 'TEST_CONFIG_KEY'

NEW_VALUE = 'new_value'

KILL_SWITCH_DISABLED_MARK = pytest.mark.config(**{CONFIG_KEY: dynconf.USE_STATIC_DEFAULT})
