from . import constants


@constants.KILL_SWITCH_DISABLED_MARK
async def test_kill_switch_disabling_using_mark(
    clean_cache_and_flush_testpoint,
    check_no_update,
):
    await check_no_update()


async def test_kill_switch_disabling_using_fixture(
    clean_cache_and_flush_testpoint,
    dynamic_config,
    check_no_update,
):
    dynamic_config.switch_to_static_default(constants.CONFIG_KEY)
    await check_no_update()


async def test_kill_switch_enabling_using_fixture(
    clean_cache_and_flush_testpoint,
    dynamic_config,
    check_no_update,
):
    dynamic_config.switch_to_dynamic_value(constants.CONFIG_KEY)
    await check_no_update()
