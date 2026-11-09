import contextlib
from types import SimpleNamespace
from unittest.mock import Mock

import pytest
from pytest_userver.plugins.ydb import ydbsupport


def test_cleanup_propagates_errors():
    client = Mock()
    client.execute.side_effect = RuntimeError('Failed to clear records')

    @contextlib.contextmanager
    def client_pool():
        yield client

    request = SimpleNamespace(node=SimpleNamespace(iter_markers=lambda name: []))
    with pytest.raises(RuntimeError, match='Failed to clear records'):
        ydbsupport._ydb_init.__wrapped__(
            request=request,
            _ydb_client=client,
            _ydb_state=None,
            ydb_service_settings=None,
            _ydb_prepare=None,
            _ydb_tables=('records',),
            _ydb_client_pool=client_pool,
            load=None,
        )
    client.execute.assert_called_once_with('DELETE FROM `records`')
