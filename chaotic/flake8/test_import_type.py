from library.python.testing import flake8
import yatest.common


def test_import_type():
    chaotic_source_path = yatest.common.test_source_path('..')
    flake8.run_flake8(source_path=chaotic_source_path)
