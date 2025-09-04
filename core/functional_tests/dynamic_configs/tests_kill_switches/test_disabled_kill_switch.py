from . import constants


@constants.KILL_SWITCH_DISABLED_MARK
async def test_duplicate_value(
    clean_cache_and_flush_testpoint,
    dynamic_config,
    check_no_update,
):
    dynamic_config.set_values({constants.CONFIG_KEY: dynamic_config.get(constants.CONFIG_KEY)})
    await check_no_update()


@constants.KILL_SWITCH_DISABLED_MARK
async def test_change_value(
    clean_cache_and_flush_testpoint,
    dynamic_config,
    check_no_update,
):
    dynamic_config.set_values({constants.CONFIG_KEY: constants.NEW_VALUE})
    dynamic_config.switch_to_static_default(constants.CONFIG_KEY)
    await check_no_update()


@constants.KILL_SWITCH_DISABLED_MARK
async def test_change_flag(
    clean_cache_and_flush_testpoint,
    dynamic_config,
    check_fresh_update,
):
    dynamic_config.switch_to_dynamic_value(constants.CONFIG_KEY)
    await check_fresh_update()


@constants.KILL_SWITCH_DISABLED_MARK
async def test_change_value_and_flag(
    clean_cache_and_flush_testpoint,
    dynamic_config,
    check_fresh_update,
):
    dynamic_config.set_values({constants.CONFIG_KEY: constants.NEW_VALUE})
    await check_fresh_update()
