import pathlib

import pytest
from tests.golden_tests.helpers import utils

TESTS_DIR = pathlib.Path(__file__).resolve().parent
INPUT_DIR = TESTS_DIR / 'input'
OUTPUT_DIR = TESTS_DIR / 'output'


@pytest.mark.parametrize('name', utils.list_cases(INPUT_DIR))
def test_golden(name: str, tmp_path: pathlib.Path) -> None:
    case = utils.Case(input_dir=INPUT_DIR / name, output_dir=tmp_path)
    case.run()

    diff = utils.dirs_diff(expected_dir=OUTPUT_DIR / name, actual_dir=tmp_path)
    assert not diff, f'{name} differs from golden:\n{diff}'
