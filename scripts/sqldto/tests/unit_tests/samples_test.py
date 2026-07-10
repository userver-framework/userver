import pytest

from sqldto.dumper.postgres import samples
from sqldto.shared.types import models


def param(pg_type: str | None, nullable: bool = False) -> models.QueryParam:
    return models.QueryParam(
        type=pg_type,
        nullable=nullable,
    )


def make_catalog(
    types: list[models.StructType] | None = None,
    enums: list[models.DbEnum] | None = None,
) -> models.Catalog:
    return models.Catalog(
        tables={},
        types={t.db_name: t for t in (types or [])},
        enums={e.db_name: e for e in (enums or [])},
    )


@pytest.fixture
def empty_catalog() -> models.Catalog:
    return make_catalog()


class TestPrimitives:
    @pytest.mark.parametrize(
        'pg_type, expected',
        [
            ('boolean', 'true'),
            ('integer', '0'),
            ('bigint', '0'),
            ('real', '0'),
            ('numeric(10, 2)', '0'),
            ('text', "''"),
            ('character varying(50)', "''"),
            ('date', "'2000-01-01'"),
            ('timestamp with time zone', "'2000-01-01 00:00:00+00'"),
            ('uuid', "'00000000-0000-0000-0000-000000000000'"),
            ('jsonb', "'null'"),
            ('bytea', r"'\\x'"),
        ],
    )
    def test_known_types(self, pg_type, expected, empty_catalog):
        assert samples.sample(param(pg_type), empty_catalog) == expected

    def test_unknown_type(self, empty_catalog):
        assert samples.sample(param('made_up'), empty_catalog) == 'NULL'

    def test_missing_type(self, empty_catalog):
        assert samples.sample(param(None), empty_catalog) == 'NULL'

    def test_nullable_overrides_type(self, empty_catalog):
        assert samples.sample(param('integer', nullable=True), empty_catalog) == 'NULL'


class TestArrays:
    def test_primitive_array(self, empty_catalog):
        assert samples.sample(param('integer[]'), empty_catalog) == "'{}'"

    def test_nullable_array(self, empty_catalog):
        assert samples.sample(param('text[]', nullable=True), empty_catalog) == 'NULL'


class TestEnums:
    def test_enum_picks_first(self):
        catalog = make_catalog(
            enums=[
                models.DbEnum(
                    namespace='public',
                    name='status',
                    values=['active', 'blocked', 'deleted'],
                ),
            ],
        )
        assert samples.sample(param('public.status'), catalog) == "'active'"

    def test_nullable_enum(self):
        catalog = make_catalog(
            enums=[
                models.DbEnum(
                    namespace='public',
                    name='status',
                    values=['active'],
                ),
            ],
        )
        assert samples.sample(param('public.status', nullable=True), catalog) == 'NULL'


class TestComposite:
    def test_flat_struct(self):
        catalog = make_catalog(
            types=[
                models.StructType(
                    namespace='public',
                    name='point',
                    fields=[
                        models.Column(name='x', type='integer', nullable=False),
                        models.Column(name='y', type='integer', nullable=False),
                    ],
                ),
            ],
        )
        assert samples.sample(param('public.point'), catalog) == 'ROW(0, 0)'

    def test_mixed_field_types(self):
        catalog = make_catalog(
            types=[
                models.StructType(
                    namespace='public',
                    name='user_profile',
                    fields=[
                        models.Column(name='display_name', type='text', nullable=False),
                        models.Column(name='age', type='integer', nullable=False),
                        models.Column(name='is_verified', type='boolean', nullable=False),
                    ],
                ),
            ],
        )
        assert samples.sample(param('public.user_profile'), catalog) == "ROW('', 0, true)"

    def test_field_nullable_ignored(self):
        # `sample()` recurses into composite fields with nullable=False unconditionally,
        # so the field's own nullable flag in the catalog has no effect.
        catalog = make_catalog(
            types=[
                models.StructType(
                    namespace='public',
                    name='wrapper',
                    fields=[
                        models.Column(name='value', type='integer', nullable=True),
                    ],
                ),
            ],
        )
        assert samples.sample(param('public.wrapper'), catalog) == 'ROW(0)'

    def test_nested_struct(self):
        catalog = make_catalog(
            types=[
                models.StructType(
                    namespace='public',
                    name='inner',
                    fields=[
                        models.Column(name='a', type='integer', nullable=False),
                        models.Column(name='b', type='text', nullable=False),
                    ],
                ),
                models.StructType(
                    namespace='public',
                    name='outer',
                    fields=[
                        models.Column(name='id', type='bigint', nullable=False),
                        models.Column(name='payload', type='public.inner', nullable=False),
                    ],
                ),
            ],
        )
        assert samples.sample(param('public.outer'), catalog) == "ROW(0, ROW(0, ''))"

    def test_enum_field(self):
        catalog = make_catalog(
            types=[
                models.StructType(
                    namespace='public',
                    name='entry',
                    fields=[
                        models.Column(name='label', type='text', nullable=False),
                        models.Column(name='state', type='public.state', nullable=False),
                    ],
                ),
            ],
            enums=[
                models.DbEnum(
                    namespace='public',
                    name='state',
                    values=['new', 'done'],
                ),
            ],
        )
        assert samples.sample(param('public.entry'), catalog) == "ROW('', 'new')"

    def test_array_field(self):
        catalog = make_catalog(
            types=[
                models.StructType(
                    namespace='public',
                    name='bag',
                    fields=[
                        models.Column(name='items', type='integer[]', nullable=False),
                    ],
                ),
            ],
        )
        assert samples.sample(param('public.bag'), catalog) == "ROW('{}')"

    def test_unknown_field_type(self):
        catalog = make_catalog(
            types=[
                models.StructType(
                    namespace='public',
                    name='broken',
                    fields=[
                        models.Column(name='ok', type='integer', nullable=False),
                        models.Column(name='bogus', type='no_such_type', nullable=False),
                    ],
                ),
            ],
        )
        assert samples.sample(param('public.broken'), catalog) == 'ROW(0, NULL)'

    def test_deeply_nested(self):
        catalog = make_catalog(
            types=[
                models.StructType(
                    namespace='public',
                    name='leaf',
                    fields=[
                        models.Column(name='v', type='integer', nullable=False),
                    ],
                ),
                models.StructType(
                    namespace='public',
                    name='mid',
                    fields=[
                        models.Column(name='leaf', type='public.leaf', nullable=False),
                    ],
                ),
                models.StructType(
                    namespace='public',
                    name='root',
                    fields=[
                        models.Column(name='mid', type='public.mid', nullable=False),
                    ],
                ),
            ],
        )
        assert samples.sample(param('public.root'), catalog) == 'ROW(ROW(ROW(0)))'

    def test_empty_struct(self):
        catalog = make_catalog(
            types=[
                models.StructType(
                    namespace='public',
                    name='empty',
                    fields=[],
                ),
            ],
        )
        assert samples.sample(param('public.empty'), catalog) == 'ROW()'
