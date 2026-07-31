import pathlib
import shutil

import pytest
from tests.golden_tests.helpers import utils

GOLDEN_DIR = pathlib.Path(__file__).resolve().parent.parent
INPUT_DIR = GOLDEN_DIR / 'input'
OUTPUT_DIR = GOLDEN_DIR / 'output'


@pytest.mark.parametrize('name', utils.list_cases(INPUT_DIR))
def test_canonize(name: str, tmp_path: pathlib.Path) -> None:
    case = utils.Case(input_dir=INPUT_DIR / name, output_dir=tmp_path)
    case.dump()
    case.run()

    output_dir = OUTPUT_DIR / name
    shutil.rmtree(output_dir, ignore_errors=True)
    shutil.copytree(tmp_path, output_dir)
