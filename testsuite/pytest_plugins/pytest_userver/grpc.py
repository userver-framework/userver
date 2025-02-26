import pytest


def _guess_servicer(module):
    guesses = [member for member in dir(module) if str(member).endswith('Servicer')]
    assert len(guesses) == 1, f"Don't know what servicer to choose: {guesses}"
    return guesses[0]


def make_mock_grpc(module, *, fixture_name, servicer=None):
    def mock_session(grpc_mockserver, create_grpc_mock):
        nonlocal servicer
        if servicer is None:
            servicer = _guess_servicer(module)
        mock = create_grpc_mock(getattr(module, servicer))
        getattr(module, 'add_{}_to_server'.format(servicer))(
            mock.servicer,
            grpc_mockserver,
        )
        return mock

    def mock(request):
        mock_session = request.getfixturevalue(f'{fixture_name}_session')
        with mock_session.mock() as m:
            yield m

    return (
        pytest.fixture(mock, name=fixture_name),
        pytest.fixture(mock_session, scope='session', name=f'{fixture_name}_session'),
    )
