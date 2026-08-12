import dataclasses

import jinja2

from sqldto.generator.postgres import translator
from sqldto.shared.types import errors
from sqldto.shared.types import models
from sqldto.shared.utils import cpp_names
from sqldto.shared.utils import rendering


@dataclasses.dataclass
class EnumEntry:
    name: str
    cpp_name: str


@dataclasses.dataclass
class EnumToGenerate:
    db_name: str
    cpp_name: str
    entries: list[EnumEntry]


@dataclasses.dataclass
class StructField:
    cpp_name: str
    cpp_type: translator.CppType


@dataclasses.dataclass
class StructToGenerate:
    db_name: str
    cpp_name: str
    fields: list[StructField]


@dataclasses.dataclass
class AliasToGenerate:
    cpp_name_from: str
    cpp_name_to: str


@dataclasses.dataclass
class QueryParamToGenerate:
    cpp_type: translator.CppType


@dataclasses.dataclass
class ReturnStructToGenerate:
    cpp_name: str
    fields: list[StructField]


@dataclasses.dataclass
class QueryToGenerate:
    name: str
    cpp_name: str
    sql_name: str
    args: list[QueryParamToGenerate]
    cardinality: models.QueryCardinality
    returns: ReturnStructToGenerate


def _return_field_name(param: models.ReturnParam, index: int, used: set[str]) -> str:
    base = cpp_names.cpp_identifier_lower(param.name) if param.name else f'field{index}'

    candidate = base
    suffix = 1
    while candidate in used:
        candidate = f'{base}_{suffix}'
        suffix += 1

    used.add(candidate)
    return candidate


class PgModelsGenerator:
    def __init__(self, context: rendering.Context) -> None:
        self.context = context
        self.loader = jinja2.PackageLoader(__package__, 'templates')
        self.catalog = context.schemas.catalog

    def pg_to_cpp_type(
        self,
        pg_object: models.Column | models.StructType | models.DbEnum,
        includes_accumulator: set[str] | None = None,
    ) -> translator.CppType:
        if includes_accumulator is None:
            includes_accumulator = set()

        pg_typename: str = None
        nullable: bool = None

        if isinstance(pg_object, models.Column):
            pg_typename = pg_object.type
            nullable = pg_object.nullable

        elif isinstance(pg_object, models.StructType):
            pg_typename = pg_object.db_name
            nullable = False

        elif isinstance(pg_object, models.DbEnum):
            pg_typename = pg_object.db_name
            nullable = False

        cpp_type = translator.pg_to_cpp_type(
            pg_typename,
            nullable,
            self.catalog,
        )

        if not cpp_type:
            raise errors.UnexpectedError(f'Unknown type: {pg_typename}')

        if not all(template.value for template in cpp_type.templates):
            missing_templates = [template.name for template in cpp_type.templates if not template.value]
            raise errors.UnsupportedError(
                f"""
                Unsupported postgres type: {pg_typename} that requires templates.
                For {cpp_type.typename} need specify {','.join(missing_templates)}.
                """
            )

        includes_accumulator.update(cpp_type.includes)
        return cpp_type

    def trust_typename(self, pg_typename: str) -> bool:
        if pg_typename not in self.catalog.types:
            return True

        pg_type = self.catalog.types[pg_typename]

        for table in self.catalog.tables.values():
            if table.type_name == pg_typename and models.table_like_type(table, pg_type):
                return True

        return False

    def generate(self) -> list[rendering.ToGenerate]:
        includes_to_generate: set[str] = set()
        user_types = [pg_type for pg_type in self.catalog.sorted_types() if not self.trust_typename(pg_type.db_name)]
        table_types = [
            models.table_to_type(table)
            for table in self.catalog.sorted_tables()
            if self.trust_typename(table.type_name)
        ]

        enums_to_generate = [
            EnumToGenerate(
                db_name=enum.db_name,
                cpp_name=translator.pg_name_to_cpp_name(enum.db_name),
                entries=[
                    EnumEntry(
                        name=value,
                        cpp_name=cpp_names.cpp_enum_entry(value),
                    )
                    for value in enum.values
                ],
            )
            for enum in self.catalog.sorted_enums()
        ]

        structs_to_generate = [
            StructToGenerate(
                db_name=type_.db_name,
                cpp_name=translator.pg_name_to_cpp_name(type_.db_name),
                fields=[
                    StructField(
                        cpp_name=cpp_names.cpp_identifier_lower(field.name),
                        cpp_type=self.pg_to_cpp_type(field, includes_to_generate),
                    )
                    for field in type_.fields
                ],
            )
            for type_ in user_types + table_types
        ]

        aliases_to_generate = [
            AliasToGenerate(
                cpp_name_from=translator.pg_name_to_cpp_name(table.db_name),
                cpp_name_to=translator.pg_name_to_cpp_name(models.table_to_type(table).db_name),
            )
            for table in self.catalog.sorted_tables()
            if self.trust_typename(table.type_name)
        ]

        to_generate = rendering.ToGenerate(
            loader=self.loader,
            template_name='pg_models.hpp.jinja',
            output_file=self.context.output_headers_dir / 'pg_models.hpp',
            context={
                'namespace': self.context.namespace,
                'structs': structs_to_generate,
                'enums': enums_to_generate,
                'aliases': aliases_to_generate,
            },
            includes=list(sorted(includes_to_generate)),
            clang_format=self.context.clang_format,
        )
        return [to_generate]


class PgQueriesGenerator:
    def __init__(self, context: rendering.Context) -> None:
        self.context = context
        self.loader = jinja2.PackageLoader(__package__, 'templates')
        self.catalog = context.schemas.catalog

    def pg_to_cpp_type(
        self,
        param: models.QueryParam,
        includes_accumulator: set[str] | None = None,
    ) -> list[translator.CppType]:
        if includes_accumulator is None:
            includes_accumulator = set()

        cpp_type = translator.pg_to_cpp_type(param.type, param.nullable, self.catalog)

        if not cpp_type:
            raise errors.UnexpectedError(f'Unknown type: {param.type}')

        if not all(template.value for template in cpp_type.templates):
            missing_templates = [template.name for template in cpp_type.templates if not template.value]
            raise errors.UnsupportedError(
                f"""
                Unsupported postgres query parameter type: {param.type} that requires templates.
                For {cpp_type.typename} need specify {','.join(missing_templates)}.
                Or you can use @arg<N>: <type> to override the type.
                """
            )

        includes_accumulator.update(cpp_type.includes)
        return cpp_type

    def generate(self) -> list[rendering.ToGenerate]:
        if not self.context.schemas.queries:
            return []

        includes_to_generate: set[str] = {
            'userver/storages/postgres/cluster.hpp',
            'userver/storages/postgres/postgres_fwd.hpp',
        }

        queries_to_generate: list[QueryToGenerate] = []
        for query in self.context.schemas.queries:
            args = [
                QueryParamToGenerate(
                    cpp_type=self.pg_to_cpp_type(param, includes_to_generate),
                )
                for param in query.args
            ]

            used_field_names: set[str] = set()
            returns = ReturnStructToGenerate(
                cpp_name=cpp_names.cpp_identifier_camel_case(query.name) + 'Row',
                fields=[
                    StructField(
                        cpp_name=_return_field_name(param, index, used_field_names),
                        cpp_type=self.pg_to_cpp_type(param, includes_to_generate),
                    )
                    for index, param in enumerate(query.returns)
                ],
            )

            if len(returns.fields) > 1:
                includes_to_generate.add(
                    'userver/storages/postgres/io/row_types.hpp',
                )

            queries_to_generate.append(
                QueryToGenerate(
                    name=query.name,
                    cpp_name=cpp_names.cpp_identifier_camel_case(query.name),
                    sql_name=cpp_names.cpp_enum_entry(query.name),
                    args=args,
                    cardinality=query.cardinality,
                    returns=returns,
                )
            )

        namespace = self.context.namespace
        type_includes = list(sorted(includes_to_generate))

        context = {
            'namespace': namespace,
            'queries': queries_to_generate,
        }

        return [
            rendering.ToGenerate(
                loader=self.loader,
                template_name='pg_client.hpp.jinja',
                output_file=self.context.output_headers_dir / 'pg_client.hpp',
                context=context,
                includes=type_includes + [f'{namespace}/pg_models.hpp'],
                clang_format=self.context.clang_format,
            ),
            rendering.ToGenerate(
                loader=self.loader,
                template_name='pg_cluster.hpp.jinja',
                output_file=self.context.output_headers_dir / 'pg_cluster.hpp',
                context=context,
                includes=[f'{namespace}/pg_client.hpp'],
                clang_format=self.context.clang_format,
            ),
            rendering.ToGenerate(
                loader=self.loader,
                template_name='pg_mock.hpp.jinja',
                output_file=self.context.output_headers_dir / 'pg_mock.hpp',
                context=context,
                includes=['gmock/gmock.h', f'{namespace}/pg_client.hpp'],
                clang_format=self.context.clang_format,
            ),
            rendering.ToGenerate(
                loader=self.loader,
                template_name='pg_cluster.cpp.jinja',
                output_file=self.context.output_sources_dir / 'pg_cluster.cpp',
                context=context,
                includes=[f'{namespace}/sql_queries.hpp', f'{namespace}/pg_cluster.hpp'],
                clang_format=self.context.clang_format,
            ),
        ]
