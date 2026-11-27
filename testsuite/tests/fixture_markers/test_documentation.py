"""Executable snippets for the Doxygen fixture-marker guide."""

# [fixture markers custom mark]
import dataclasses

import pytest
from pytest_userver import fixture_markers


@dataclasses.dataclass(frozen=True)
class PgInitializer:
    order: int


def pg_init(*, order: int = 0):
    def decorate(function):
        return fixture_markers.mark(function, PgInitializer(order=order))

    return decorate


# [fixture markers custom mark]


@pytest.fixture
def initialized_tables():
    # Stand-in for database initialization, so the examples need no database.
    return []


# [fixture markers definitions]
@pytest.fixture
@pg_init(order=1)
def init_users(initialized_tables):
    initialized_tables.append('users')


@pytest.fixture(name='init_orders')
@pg_init(order=2)
def _init_orders(initialized_tables):
    initialized_tables.append('orders')


# [fixture markers definitions]


# [fixture markers discovery]
def test_initializers(request, initialized_tables):
    initializers = fixture_markers.get_infos(request, PgInitializer)
    assert initialized_tables == []  # Discovery does not execute fixtures.
    assert initializers == {
        'init_users': PgInitializer(order=1),
        'init_orders': PgInitializer(order=2),
    }

    # A consumer can choose an explicit priority instead of definition order.
    for name, info in sorted(initializers.items(), key=lambda item: item[1].order):
        request.getfixturevalue(name)

    assert initialized_tables == ['users', 'orders']


# [fixture markers discovery]


class TestInheritedMark:
    # [fixture markers inherited override]
    @pytest.fixture
    def init_users(self, init_users, initialized_tables):
        initialized_tables.append('guests')

    def test_inherited_mark(self, request, initialized_tables):
        infos = fixture_markers.get_infos(request, PgInitializer)
        assert infos['init_users'] == PgInitializer(order=1)
        request.getfixturevalue('init_users')
        assert initialized_tables == ['users', 'guests']

    # [fixture markers inherited override]


class TestReplacedMark:
    # [fixture markers replaced override]
    @pytest.fixture
    @pg_init(order=10)
    def init_users(self, initialized_tables):
        initialized_tables.append('replacement-users')

    def test_replaced_mark(self, request, initialized_tables):
        infos = fixture_markers.get_infos(request, PgInitializer)
        assert infos['init_users'] == PgInitializer(order=10)
        # Replacing metadata does not move the fixture's original position.
        assert list(infos) == ['init_users', 'init_orders']
        request.getfixturevalue('init_users')
        assert initialized_tables == ['replacement-users']

    # [fixture markers replaced override]
