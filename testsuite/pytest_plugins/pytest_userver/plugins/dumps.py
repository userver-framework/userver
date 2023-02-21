import pathlib
import shutil
import typing

import pytest

USERVER_CONFIG_HOOKS = ['_userver_config_dumps']


@pytest.fixture(scope='session')
def userver_dumps_root(tmp_path_factory) -> pathlib.Path:
    """
    The directory which the service will use for cache dumps.
    Dumps of individual components will be stored in
    `{userver_dumps_root}/{component-name}/{datetime}-v{format-version}`

    @see userver::dump::Dumper
    @ingroup userver_testsuite_fixtures
    """
    path = tmp_path_factory.mktemp('userver-cache-dumps', numbered=True)
    return path.resolve()


@pytest.fixture
def read_latest_dump(userver_dumps_root):
    """
    Read the latest dump produced by a specified dumper.

    @see userver::dump::Dumper
    @ingroup userver_testsuite_fixtures
    """

    def read(dumper_name: str) -> typing.Optional[bytes]:
        specific_dir = userver_dumps_root.joinpath(dumper_name)
        if not specific_dir.is_dir():
            return None
        latest_dump_filename = max(
            (
                f
                for f in specific_dir.iterdir()
                if specific_dir.joinpath(f).is_file()
            ),
            default=None,
        )
        if not latest_dump_filename:
            return None
        with open(specific_dir.joinpath(latest_dump_filename), 'rb') as dump:
            return dump.read()

    return read


@pytest.fixture
def cleanup_userver_dumps(userver_dumps_root, request):
    """
    To avoid leaking dumps between tests, cache_dump_dir must be cleaned after
    each test. To observe the dumps, add a final `time.sleep(1000000)` to your
    test locally. The returned function may also be used to clean dumps
    manually as appropriate.

    @see userver::dump::Dumper
    @ingroup userver_testsuite_fixtures
    """

    def cleanup() -> None:
        shutil.rmtree(userver_dumps_root, ignore_errors=True)
        userver_dumps_root.mkdir()

    request.addfinalizer(cleanup)
    return cleanup


@pytest.fixture(scope='session')
def _userver_config_dumps(pytestconfig, userver_dumps_root):
    def patch_config(_config, config_vars) -> None:
        config_vars['userver-dumps-root'] = str(userver_dumps_root)
        if not pytestconfig.getoption('--service-runner-mode', False):
            config_vars['userver-dumps-periodic'] = False

    return patch_config
