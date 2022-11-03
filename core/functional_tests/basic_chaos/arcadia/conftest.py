import os

import pytest

import yatest.common  # pylint: disable=import-error


@pytest.fixture(scope='session')
def service_binary():
    path = os.path.relpath(
        yatest.common.test_source_path('../userver-core-tests-basic-chaos'),
        yatest.common.source_path(),
    )
    return yatest.common.binary_path(path)
