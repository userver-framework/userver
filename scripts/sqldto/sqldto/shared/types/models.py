from dataclasses import field
from enum import Enum
import pathlib

from pydantic import TypeAdapter
from pydantic.dataclasses import dataclass

from sqldto.shared.types import errors


class Dialect(str, Enum):
    postgresql = 'postgresql'

    @staticmethod
    def parse(arg: str) -> 'Dialect':
        try:
            return Dialect(arg)
        except ValueError:
            raise errors.ClientError(f'Unknown dialect: {arg}')


@dataclass(order=True)
class Migration:
    path: pathlib.Path
    version: str
    sql: str

    def next_version(self) -> str:
        digits: int = len(self.version)
        version: int = int(self.version)
        return f'{version + 1:0{digits}d}'


class QueryCardinality(str, Enum):
    void = 'void'
    one = 'one'
    many = 'many'
    optional = 'optional'


@dataclass(order=True)
class QueryParam:
    type: str | None  # None if not inferred
    nullable: bool | None  # None if not inferred


@dataclass(order=True)
class ReturnParam:
    type: str | None  # None if not inferred
    nullable: bool | None  # None if not inferred
    name: str | None  # None if not inferred


@dataclass(order=True)
class Query:
    path: pathlib.Path
    sql: str
    source: str  # relative path
    no_codegen: bool
    args: list[QueryParam]
    cardinality: QueryCardinality | None  # None if not inferred


@dataclass(order=True)
class QuerySchema:
    name: str
    args: list[QueryParam]
    returns: list[ReturnParam]
    cardinality: QueryCardinality


@dataclass(order=True)
class Column:
    name: str
    type: str
    nullable: bool


@dataclass(order=True)
class Table:
    namespace: str
    name: str
    columns: list[Column]
    version: int = field(compare=False)

    @property
    def db_name(self) -> str:
        return f'{self.namespace}.{self.name}'

    @property
    def type_name(self) -> str:
        return f'{self.db_name}_v{self.version}'


@dataclass(order=True)
class StructType:
    namespace: str
    name: str
    fields: list[Column]

    @property
    def db_name(self) -> str:
        return f'{self.namespace}.{self.name}'


@dataclass(order=True)
class DbEnum:
    namespace: str
    name: str
    values: list[str]

    @property
    def db_name(self) -> str:
        return f'{self.namespace}.{self.name}'


@dataclass
class Catalog:
    tables: dict[str, Table]
    types: dict[str, StructType]
    enums: dict[str, DbEnum]

    def sorted_enums(self) -> list[DbEnum]:
        return sorted(self.enums.values(), key=lambda x: x.db_name)

    def sorted_tables(self) -> list[Table]:
        return sorted(self.tables.values(), key=lambda x: x.db_name)

    def sorted_types(self) -> list[StructType]:
        result = []
        remaining = sorted(self.types.values(), key=lambda x: x.db_name)

        while remaining:
            for remain in remaining:
                depends = {field.type for field in remain.fields if field.type in self.types}
                if depends.issubset({type_.db_name for type_ in result}):
                    result.append(remain)
                    remaining.remove(remain)
                    break
            else:
                names = [remain.db_name for remain in remaining]
                raise errors.UnexpectedError(f'Cycle detected in composite types: {names}')

        return result


@dataclass(order=True)
class Schema:
    hashsum: str
    catalog: Catalog
    queries: list[QuerySchema]

    def to_dict(self) -> dict:
        return TypeAdapter(Schema).dump_python(self, mode='json')

    @classmethod
    def from_dict(cls, data: dict):
        return TypeAdapter(Schema).validate_python(data)


def table_to_type(table: Table) -> StructType:
    return StructType(
        namespace=table.namespace,
        name=f'{table.name}_v{table.version}',
        fields=table.columns,
    )


def table_like_type(table: Table, type_: StructType) -> bool:
    table_fields = sorted(table.columns, key=lambda v: v.name)
    type_fields = sorted(type_.fields, key=lambda v: v.name)

    if len(table_fields) != len(type_fields):
        return False

    for table_field, type_field in zip(table_fields, type_fields):
        if (table_field.name, table_field.type) != (type_field.name, type_field.type):
            return False

    return True
