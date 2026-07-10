import pytest

from sqldto.generator.postgres import translator
from sqldto.shared.types import models


@pytest.fixture
def empty_catalog() -> models.Catalog:
    return models.Catalog(tables={}, types={}, enums={})


@pytest.fixture
def catalog_with_user_types() -> models.Catalog:
    enum = models.DbEnum(
        namespace='public',
        name='user_status',
        values=['active', 'blocked'],
    )
    struct = models.StructType(
        namespace='public',
        name='user_profile',
        fields=[
            models.Column(name='display_name', type='text', nullable=True),
            models.Column(name='age', type='integer', nullable=True),
        ],
    )
    return models.Catalog(
        tables={},
        types={struct.db_name: struct},
        enums={enum.db_name: enum},
    )


class TestStdTypes:
    @pytest.mark.parametrize(
        'pg_type, expected_typename',
        [
            ('boolean', 'bool'),
            ('smallint', 'std::int16_t'),
            ('integer', 'std::int32_t'),
            ('bigint', 'std::int64_t'),
            ('smallserial', 'std::int16_t'),
            ('serial', 'std::int32_t'),
            ('bigserial', 'std::int64_t'),
            ('real', 'float'),
            ('double precision', 'double'),
            ('text', 'std::string'),
            ('char', 'char'),
            ('bytea', 'std::string'),
            ('interval', 'std::chrono::microseconds'),
            ('json', 'USERVER_NAMESPACE::formats::json::Value'),
            ('jsonb', 'USERVER_NAMESPACE::formats::json::Value'),
            ('uuid', 'boost::uuids::uuid'),
            ('int4range', 'USERVER_NAMESPACE::storages::postgres::IntegerRange'),
            ('int8range', 'USERVER_NAMESPACE::storages::postgres::BigintRange'),
            ('varchar(50)', 'std::string'),
            ('varchar', 'std::string'),
            ('character varying(50)', 'std::string'),
            ('character varying', 'std::string'),
            ('character(10)', 'std::string'),
            ('character', 'std::string'),
            ('timestamp', 'USERVER_NAMESPACE::storages::postgres::TimePointWithoutTz'),
            ('timestamp without time zone', 'USERVER_NAMESPACE::storages::postgres::TimePointWithoutTz'),
            ('timestamp with time zone', 'USERVER_NAMESPACE::storages::postgres::TimePointTz'),
            ('timestamptz', 'USERVER_NAMESPACE::storages::postgres::TimePointTz'),
            ('date', 'USERVER_NAMESPACE::storages::postgres::Date'),
            ('time', 'USERVER_NAMESPACE::storages::postgres::TimeOfDay'),
            ('time without time zone', 'USERVER_NAMESPACE::storages::postgres::TimeOfDay'),
            ('time with time zone', 'USERVER_NAMESPACE::storages::postgres::TimeOfDayTz'),
            ('timetz', 'USERVER_NAMESPACE::storages::postgres::TimeOfDayTz'),
        ],
    )
    def test_mapping(self, pg_type, expected_typename):
        result = translator.pg_to_std_cpp_type(pg_type)
        assert result is not None
        assert result.typename == expected_typename

    @pytest.mark.parametrize(
        'pg_type, expected_precision',
        [
            ('numeric(10, 2)', '2'),
            ('decimal(10, 4)', '4'),
            ('numeric(5,0)', '0'),
        ],
    )
    def test_decimal(self, pg_type, expected_precision):
        result = translator.pg_to_std_cpp_type(pg_type)
        assert result is not None
        assert result.typename == 'USERVER_NAMESPACE::decimal64::Decimal'
        assert len(result.templates) == 1
        assert result.templates[0].value == expected_precision

    def test_unknown_type(self):
        assert translator.pg_to_std_cpp_type('totally_made_up_type') is None


class TestPgToCppType:
    def test_nullable(self, empty_catalog):
        result = translator.pg_to_cpp_type(
            'integer',
            nullable=True,
            catalog=empty_catalog,
        )
        assert result is not None
        assert result.typename == 'std::int32_t'
        assert result.wrappers == ['std::optional']

    def test_non_nullable(self, empty_catalog):
        result = translator.pg_to_cpp_type(
            'integer',
            nullable=False,
            catalog=empty_catalog,
        )
        assert result is not None
        assert result.wrappers == []

    def test_array(self, empty_catalog):
        result = translator.pg_to_cpp_type(
            'integer[]',
            nullable=False,
            catalog=empty_catalog,
        )
        assert result is not None
        assert result.typename == 'std::int32_t'
        assert result.wrappers == ['std::vector']

    def test_nullable_array(self, empty_catalog):
        result = translator.pg_to_cpp_type(
            'text[]',
            nullable=True,
            catalog=empty_catalog,
        )
        assert result is not None
        assert result.typename == 'std::string'
        assert result.wrappers == ['std::optional', 'std::vector']

    def test_user_enum(self, catalog_with_user_types):
        result = translator.pg_to_cpp_type(
            'public.user_status',
            nullable=False,
            catalog=catalog_with_user_types,
        )
        assert result is not None
        assert result.typename == 'PublicUserStatus'

    def test_user_composite_struct(self, catalog_with_user_types):
        result = translator.pg_to_cpp_type(
            'public.user_profile',
            nullable=False,
            catalog=catalog_with_user_types,
        )
        assert result is not None
        assert result.typename == 'PublicUserProfile'

    def test_user_enum_nullable(self, catalog_with_user_types):
        result = translator.pg_to_cpp_type(
            'public.user_status',
            nullable=True,
            catalog=catalog_with_user_types,
        )
        assert result is not None
        assert result.wrappers == ['std::optional']

    def test_unknown_type(self, empty_catalog):
        assert (
            translator.pg_to_cpp_type(
                'mystery_type',
                nullable=False,
                catalog=empty_catalog,
            )
            is None
        )
