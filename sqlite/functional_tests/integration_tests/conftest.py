import pytest
from pathlib import Path

pytest_plugins = ['pytest_userver.plugins.core']

@pytest.fixture
def sqlite_db(request):
    params = request.param
    db_path_str = params["db_path"]
    create_file = params.get("create_file", False)

    db_path = Path(db_path_str)
    if create_file:
        db_path.touch()

    yield db_path

    db_path_wal = Path(db_path_str + "-wal")
    db_path_shm = Path(db_path_str + "-shm")
    if db_path.exists():
        db_path.unlink()
    if db_path_wal.exists():
        db_path_wal.unlink()
    if db_path_shm.exists():
        db_path_shm.unlink()
