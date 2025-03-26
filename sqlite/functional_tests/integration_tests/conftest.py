import pytest
import shutil
import tempfile
import sqlite3
from pathlib import Path

pytest_plugins = ['pytest_userver.plugins.core']

@pytest.fixture
def sqlite_db(request):
    params = request.param
    db_path_str = params["db_path"]
    create_file = params.get("create_file", False)

    tmp_dir = Path(tempfile.mkdtemp())
    db_path = tmp_dir / db_path_str

    if create_file:
        db_path.parent.mkdir(parents=True, exist_ok=True)
        con = sqlite3.connect(str(db_path))
        con.close()

    yield db_path

    if tmp_dir.exists():
        shutil.rmtree(tmp_dir)
