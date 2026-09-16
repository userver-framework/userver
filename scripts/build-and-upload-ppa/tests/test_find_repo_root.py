import pathlib
import runpy

import pytest
import yatest.common

MODULE = runpy.run_path(
    yatest.common.source_path('taxi/uservices/userver/scripts/build-and-upload-ppa/__main__.py'),
)
find_repo_root = MODULE['find_repo_root']
compute_version = MODULE['compute_version']


def create_repo(root: pathlib.Path) -> None:
    (root / 'version.txt').touch()
    generator = root / 'scripts/generate-debian-directory.sh'
    generator.parent.mkdir()
    generator.touch()


def test_from_repo_root(tmp_path: pathlib.Path, monkeypatch: pytest.MonkeyPatch) -> None:
    create_repo(tmp_path)
    monkeypatch.chdir(tmp_path)

    assert find_repo_root() == tmp_path


def test_from_child_directory(tmp_path: pathlib.Path, monkeypatch: pytest.MonkeyPatch) -> None:
    create_repo(tmp_path)
    child = tmp_path / 'some/nested/directory'
    child.mkdir(parents=True)
    monkeypatch.chdir(child)

    assert find_repo_root() == tmp_path


def test_without_version_file(tmp_path: pathlib.Path, monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.chdir(tmp_path)

    with pytest.raises(SystemExit, match='version.txt is missing'):
        find_repo_root()


def test_without_generator(tmp_path: pathlib.Path, monkeypatch: pytest.MonkeyPatch) -> None:
    (tmp_path / 'version.txt').touch()
    monkeypatch.chdir(tmp_path)

    with pytest.raises(SystemExit, match='scripts/generate-debian-directory.sh'):
        find_repo_root()


def test_release_version_is_valid_for_native_package(tmp_path: pathlib.Path) -> None:
    (tmp_path / 'version.txt').write_text('3.2-rc\n')

    assert compute_version(tmp_path, 'release') == '3.2~rc'


def test_nightly_version_is_valid_for_native_package(tmp_path: pathlib.Path) -> None:
    (tmp_path / 'version.txt').write_text('3.2-rc\n')

    version = compute_version(tmp_path, 'nightly')

    assert version.startswith('3.2~rc~')
    assert '-' not in version
