import pytest


@pytest.fixture
def initial_kill_switch_flag():
    return False


async def test_duplicate_value(
    clean_cache_and_flush_testpoint,
    mock_kill_switch_storage,
    check_no_update,
):
    mock_kill_switch_storage.config_value = mock_kill_switch_storage.config_value
    await check_no_update()


async def test_change_value(
    clean_cache_and_flush_testpoint,
    mock_kill_switch_storage,
    check_no_update,
):
    mock_kill_switch_storage.config_value = 'new_value'
    await check_no_update()


async def test_change_flag(
    clean_cache_and_flush_testpoint,
    mock_kill_switch_storage,
    check_fresh_update,
):
    mock_kill_switch_storage.is_enabled = True
    await check_fresh_update()


async def test_change_value_and_flag(
    clean_cache_and_flush_testpoint,
    mock_kill_switch_storage,
    check_fresh_update,
):
    mock_kill_switch_storage.is_enabled = True
    mock_kill_switch_storage.config_value = 'new_value'
    await check_fresh_update()
