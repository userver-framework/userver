import os
import sys
from testsuite.databases.pgsql import discover
import pytest
import pathlib

sys.path.append(os.path.join(os.path.dirname(__file__), 'helpers'))

pytest_plugins = [
    "pytest_userver.plugins",
    "testsuite.databases.pgsql.pytest_plugin",
]


@pytest.fixture(scope="session")
def root_dir():
    """Path to root directory service."""
    return pathlib.Path(__file__).parent.parent


@pytest.fixture(scope="session")
def initial_data_path(root_dir):
    """Path for find files with data"""
    return [
        root_dir / "postgresql/data",
    ]


@pytest.fixture(scope="session")
def pgsql_local(root_dir, pgsql_local_create):
    """Create schemas databases for tests"""
    databases = discover.find_schemas(
        "realmedium",  # service name that goes to the DB connection
        [root_dir.joinpath("postgresql/schemas")],
    )
    return pgsql_local_create(list(databases.values()))


@pytest.fixture
def client_deps(pgsql):
    pass
