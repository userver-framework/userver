from dataclasses import dataclass
from dataclasses import field
import re

from sqldto.shared.types import models
from sqldto.shared.utils import cpp_names


@dataclass
class CppTemplate:
    typename: str
    name: str
    value: str | None


@dataclass
class CppType:
    typename: str
    explicit_default: bool = False
    includes: list[str] = field(default_factory=list)
    templates: list[CppTemplate] = field(default_factory=list)
    wrappers: list[str] = field(default_factory=list)

    def wrap_by_optional(self) -> None:
        self.explicit_default = False
        self.wrappers = ['std::optional'] + self.wrappers
        self.includes.append('userver/storages/postgres/io/optional.hpp')

    def wrap_by_vector(self) -> None:
        self.explicit_default = False
        self.wrappers = ['std::vector'] + self.wrappers
        self.includes.append('userver/storages/postgres/io/array_types.hpp')


def pg_name_to_cpp_name(name: str) -> str:
    return cpp_names.cpp_identifier_camel_case(name)


def pg_to_cpp_type(
    pg_typename: str,
    nullable: bool,
    catalog: models.Catalog,
) -> CppType | None:
    cpp_type: CppType | None = None

    if pg_typename.endswith('[]'):
        if cpp_type := pg_to_cpp_type(pg_typename[:-2], False, catalog):
            cpp_type.wrap_by_vector()

    elif cpp_type := pg_to_std_cpp_type(pg_typename):  # TODO: custom types
        pass

    elif pg_typename in catalog.enums:
        cpp_type = CppType(
            typename=pg_name_to_cpp_name(pg_typename),
            includes=['userver/storages/postgres/io/enum_types.hpp'],
        )

    elif pg_typename in catalog.types:
        cpp_type = CppType(
            typename=pg_name_to_cpp_name(pg_typename),
            includes=['userver/storages/postgres/io/io_fwd.hpp'],
        )

    if cpp_type and nullable:
        cpp_type.wrap_by_optional()

    return cpp_type


def pg_to_std_cpp_type(pg_type: str) -> CppType | None:
    if cpp_type := pg_to_floating_cpp_type(pg_type):
        return cpp_type

    if cpp_type := pg_to_integral_cpp_type(pg_type):
        return cpp_type

    if cpp_type := pg_to_str_cpp_type(pg_type):
        return cpp_type

    if cpp_type := pg_to_decimal_cpp_type(pg_type):
        return cpp_type

    if cpp_type := pg_to_chrono_cpp_type(pg_type):
        return cpp_type

    if cpp_type := pg_to_json_cpp_type(pg_type):
        return cpp_type

    if cpp_type := pg_to_binary_cpp_type(pg_type):
        return cpp_type

    if cpp_type := pg_to_range_cpp_type(pg_type):
        return cpp_type

    if cpp_type := pg_to_uuid_cpp_type(pg_type):
        return cpp_type

    return None


def pg_to_floating_cpp_type(pg_type: str) -> CppType | None:
    mapping = {
        'real': 'float',
        'double precision': 'double',
    }

    if pg_type in mapping:
        return CppType(
            typename=mapping[pg_type],
            explicit_default=True,
            includes=['userver/storages/postgres/io/floating_point_types.hpp'],
        )

    return None


def pg_to_integral_cpp_type(pg_type: str) -> CppType | None:
    mapping = {
        'boolean': 'bool',
        'smallint': 'std::int16_t',
        'integer': 'std::int32_t',
        'bigint': 'std::int64_t',
        'smallserial': 'std::int16_t',
        'serial': 'std::int32_t',
        'bigserial': 'std::int64_t',
        'char': 'char',
    }

    if pg_type in mapping:
        return CppType(
            typename=mapping[pg_type],
            explicit_default=True,
            includes=['userver/storages/postgres/io/integral_types.hpp'],
        )

    return None


def pg_to_str_cpp_type(pg_type: str) -> CppType | None:
    mapping = {
        re.compile(r'^text$'): r'std::string',
        re.compile(r'^character(\s*\(\s*(\d+)\s*\))?$'): r'std::string',
        re.compile(r'^character varying(\s*\(\s*(\d+)\s*\))?$'): r'std::string',
        re.compile(r'^varchar(\s*\(\s*(\d+)\s*\))?$'): r'std::string',
    }

    for pattern, cpp_type in mapping.items():
        matched = pattern.fullmatch(pg_type)
        if matched:
            return CppType(
                typename=matched.expand(cpp_type),
                includes=['userver/storages/postgres/io/string_types.hpp'],
            )

    return None


def pg_to_decimal_cpp_type(pg_type: str) -> CppType | None:
    mapping_with_precision = {
        re.compile(r'^numeric\s*\(\s*(\d+)\s*,\s*(\d+)\s*\)$'): r'USERVER_NAMESPACE::decimal64::Decimal',
        re.compile(r'^decimal\s*\(\s*(\d+)\s*,\s*(\d+)\s*\)$'): r'USERVER_NAMESPACE::decimal64::Decimal',
    }
    mapping_without_precision = {
        re.compile(r'^numeric$'): r'USERVER_NAMESPACE::decimal64::Decimal',
        re.compile(r'^decimal$'): r'USERVER_NAMESPACE::decimal64::Decimal',
    }

    precision = r'\2'
    for pattern, cpp_type in mapping_with_precision.items():
        matched = pattern.fullmatch(pg_type)
        if matched:
            return CppType(
                typename=matched.expand(cpp_type),
                includes=['userver/storages/postgres/io/decimal64.hpp'],
                templates=[
                    CppTemplate(
                        typename='int',
                        name='Prec',
                        value=matched.expand(precision),
                    ),
                ],
            )

    for pattern, cpp_type in mapping_without_precision.items():
        matched = pattern.fullmatch(pg_type)
        if matched:
            return CppType(
                typename=matched.expand(cpp_type),
                includes=['userver/storages/postgres/io/decimal64.hpp'],
                templates=[
                    CppTemplate(
                        typename='int',
                        name='Prec',
                        value=None,
                    ),
                ],
            )

    return None


def pg_to_chrono_cpp_type(pg_type: str) -> CppType | None:
    mapping = {
        'timestamp without time zone': 'USERVER_NAMESPACE::storages::postgres::TimePointWithoutTz',
        'timestamp': 'USERVER_NAMESPACE::storages::postgres::TimePointWithoutTz',
        'timestamp with time zone': 'USERVER_NAMESPACE::storages::postgres::TimePointTz',
        'timestamptz': 'USERVER_NAMESPACE::storages::postgres::TimePointTz',
        'date': 'USERVER_NAMESPACE::storages::postgres::Date',
        'time': 'USERVER_NAMESPACE::storages::postgres::TimeOfDay',
        'time without time zone': 'USERVER_NAMESPACE::storages::postgres::TimeOfDay',
        'time with time zone': 'USERVER_NAMESPACE::storages::postgres::TimeOfDayTz',
        'timetz': 'USERVER_NAMESPACE::storages::postgres::TimeOfDayTz',
        'interval': 'std::chrono::microseconds',
    }

    if pg_type in mapping:
        return CppType(
            typename=mapping[pg_type],
            includes=['userver/storages/postgres/io/chrono.hpp'],
        )

    return None


def pg_to_json_cpp_type(pg_type: str) -> CppType | None:
    mapping = {
        'json': 'USERVER_NAMESPACE::formats::json::Value',
        'jsonb': 'USERVER_NAMESPACE::formats::json::Value',
    }

    if pg_type in mapping:
        return CppType(
            typename=mapping[pg_type],
            includes=['userver/storages/postgres/io/json_types.hpp'],
        )

    return None


def pg_to_binary_cpp_type(pg_type: str) -> CppType | None:
    mapping = {
        'bytea': 'std::string',
    }

    if pg_type in mapping:
        return CppType(
            typename=mapping[pg_type],
            includes=['userver/storages/postgres/io/string_types.hpp'],
        )

    return None


def pg_to_range_cpp_type(pg_type: str) -> CppType | None:
    mapping = {
        'int4range': 'USERVER_NAMESPACE::storages::postgres::IntegerRange',
        'int8range': 'USERVER_NAMESPACE::storages::postgres::BigintRange',
    }

    if pg_type in mapping:
        return CppType(
            typename=mapping[pg_type],
            includes=['userver/storages/postgres/io/range_types.hpp'],
        )

    return None


def pg_to_uuid_cpp_type(pg_type: str) -> CppType | None:
    if pg_type == 'uuid':
        return CppType(
            typename='boost::uuids::uuid',
            includes=['userver/storages/postgres/io/uuid.hpp', 'boost/uuid/uuid.hpp'],
        )

    return None
