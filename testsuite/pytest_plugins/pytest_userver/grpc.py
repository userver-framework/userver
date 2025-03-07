from typing import List
from typing import Optional

import pytest


def _guess_servicer(module) -> str:
    guesses = [member for member in dir(module) if str(member).endswith('Servicer')]
    assert len(guesses) == 1, f"Don't know what servicer to choose: {guesses}"
    return str(guesses[0])


def make_mock_grpc(
    module,
    *,
    fixture_name: str,
    servicer: Optional[str] = None,
    stream_method_names: Optional[List[str]] = None,
):
    """
    @deprecated Use
    @ref pytest_userver.plugins.grpc.mockserver.grpc_mockserver_new "grpc_mockserver_new"
    instead.
    """

    @pytest.fixture(name=fixture_name)
    def mock_fixture(grpc_mockserver_new):
        servicer_name = servicer
        if servicer_name is None:
            servicer_name = _guess_servicer(module)
        resolved_servicer = getattr(module, servicer_name)

        return grpc_mockserver_new.mock_factory(resolved_servicer)

    # Returning a tuple for backwards compatibility.
    return None, mock_fixture
