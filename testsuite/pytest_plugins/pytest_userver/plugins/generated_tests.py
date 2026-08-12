"""
Pytest infrastructure plugin for collecting virtual generated tests.

The plugin adds a synthetic file named `__test_generated__.py` under the first
collection directory from `config.args` and exposes a custom pytest hook,
`pytest_generate_virtual_tests`.

Other plugins can implement that hook to return generated `pytest.Item`
objects, usually `pytest.Function` items.

Generated tests are not backed by a real Python file, but they get a normal
pytest parent chain and can use pytest fixtures when implemented as
`pytest.Function` objects.
"""

from collections.abc import Iterable
from collections.abc import Sequence
import pathlib

import pytest
from typing_extensions import override

try:
    import yatest.common  # noqa: F401

    _IS_ARCADIA = True
except ModuleNotFoundError:
    _IS_ARCADIA = False

_GENERATED_FILENAME = '__test_generated__.py'

_GENERATED_TESTS_DIR_KEY = pytest.StashKey[pathlib.Path | None]()
_COLLECTED_ITEMS_KEY = pytest.StashKey[list[pytest.Item]]()


class GeneratedTestsPluginHooks:
    """
    Custom pytest (pluggy) hooks customizing generated tests.

    @ingroup userver_testsuite
    """

    @pytest.hookspec
    def pytest_generate_virtual_tests(
        self,
        parent: pytest.File,
        config: pytest.Config,
        existing_items: Sequence[pytest.Item],
    ) -> Iterable[pytest.Item]:
        """
        Returns generated pytest items to be collected under the virtual `__test_generated__.py` file.
        """
        raise NotImplementedError


def pytest_addhooks(pluginmanager: pytest.PytestPluginManager) -> None:
    pluginmanager.add_hookspecs(GeneratedTestsPluginHooks)


def pytest_configure(config: pytest.Config) -> None:
    if _IS_ARCADIA:
        # Skip trying to touch disk instead of resfs.
        return

    config.stash[_GENERATED_TESTS_DIR_KEY] = _get_first_collection_directory(config)
    config.stash[_COLLECTED_ITEMS_KEY] = []


def pytest_itemcollected(item: pytest.Item) -> None:
    if _IS_ARCADIA:
        return

    # Accumulate the real items as pytest collects them. Since the wrapped
    # directory yields the virtual `__test_generated__.py` file last, every real
    # item has already fired this hook by the time the virtual file is collected.
    item.config.stash[_COLLECTED_ITEMS_KEY].append(item)


def _get_first_collection_directory(config: pytest.Config) -> pathlib.Path | None:
    if not config.args:
        return None

    base = pathlib.Path(config.invocation_params.dir).resolve()

    raw = config.args[0].split('::', 1)[0]
    path = pathlib.Path(raw)

    if not path.is_absolute():
        path = base / path

    path = path.resolve()

    if path.is_file():
        return path.parent

    if path.is_dir():
        return path

    return None


def pytest_collect_directory(path: pathlib.Path, parent: pytest.Collector):
    if _IS_ARCADIA:
        # The OSS way of injecting generated tests does not work there, because:
        # * pytest_collect_directory hook currently does not work (could be worked around)
        # * only some of the targets need injected tests (could be worked around too)
        return None

    generated_tests_dir = parent.config.stash[_GENERATED_TESTS_DIR_KEY]

    if generated_tests_dir is not None and path.resolve() == generated_tests_dir:
        return _create_wrapped_directory(parent=parent, path=path)

    return None


def _create_wrapped_directory(*, parent: pytest.Collector, path: pathlib.Path) -> pytest.Directory:
    if path.joinpath('__init__.py').is_file():
        return _GeneratedTestsPackage.from_parent(parent, path=path)

    return _GeneratedTestsDir.from_parent(parent, path=path)


class _GeneratedTestsDirectoryMixin(pytest.Directory):
    @override
    def collect(self) -> Iterable[pytest.Item | pytest.Collector]:
        # Yield the real children first and let pytest collect them normally; the
        # session fires `pytest_itemcollected` for each resulting item, and our
        # hook accumulates them into the config stash. The virtual file is yielded
        # last, so by the time it is collected all real items are already known.
        yield from super().collect()

        yield _GeneratedTestsFile.from_parent(
            self,
            path=self.path / _GENERATED_FILENAME,
        )


class _GeneratedTestsDir(_GeneratedTestsDirectoryMixin, pytest.Dir):
    pass


class _GeneratedTestsPackage(_GeneratedTestsDirectoryMixin, pytest.Package):
    pass


class _FakeModule:
    def __init__(self, *, path: pathlib.Path) -> None:
        self.__file__ = str(path)


class _GeneratedTestsFile(pytest.Module):
    @override
    def _getobj(self) -> object:
        return _FakeModule(path=self.path)

    @override
    def collect(self) -> Iterable[pytest.Item | pytest.Collector]:
        existing_items = self.config.stash[_COLLECTED_ITEMS_KEY]

        hook_results = self.config.hook.pytest_generate_virtual_tests(
            parent=self,
            config=self.config,
            existing_items=existing_items,
        )

        for items in hook_results:
            yield from items
