import copy
import uuid

from sqldto.dumper.postgres import samples
from sqldto.shared.types import errors
from sqldto.shared.types import models
from sqldto.shared.utils import logging

logger = logging.logger


class PgQueryAnalyzer:
    def __init__(self, catalog: models.Catalog):
        self.catalog: models.Catalog = catalog

    def read_schemas(
        self,
        cursor,
        queries: list[models.Query],
    ) -> list[models.QuerySchema]:
        return [self.read_schema(cursor, query) for query in queries if not query.no_codegen]

    def read_schema(
        self,
        cursor,
        query: models.Query,
    ) -> models.QuerySchema:
        try:
            params, returns = self.read_params(cursor, query, [])
        except Exception as ex:
            raise errors.RuntimeError(
                f"""
                Can't analyze query {query.path.name}.
                Often examples using UNNEST($1) or $1.field,
                try specify parameters explicitly by using comments:
                -- @arg1: TEXT
                -- ...
                -- @arg<N>: <TYPE>

                More details:
                {ex}
                """
            )

        for i in range(len(params)):
            if params[i].nullable is not None:
                continue

            params[i].nullable = True
            try:
                self.read_params(cursor, query, params)
            except Exception:
                params[i].nullable = False  # pg require not null arg

        return models.QuerySchema(
            name=query.path.stem,
            args=params,
            returns=returns,
            cardinality=query.cardinality or models.QueryCardinality.many,
        )

    def guess_return_type(self, pg_typename: str, cursor, column) -> models.ReturnParam:
        if column.table_oid is not None and column.table_column is not None:
            # TODO: conflicts with public namespace
            cursor.execute(f"""
                SELECT pg_catalog.format_type(a.atttypid, a.atttypmod)
                FROM pg_catalog.pg_attribute a
                JOIN pg_catalog.pg_type t ON t.oid = a.atttypid
                JOIN pg_catalog.pg_namespace n ON n.oid = t.typnamespace
                WHERE a.attrelid = {column.table_oid}
                AND a.attnum = {column.table_column}
                AND NOT a.attisdropped
            """)

            return models.ReturnParam(
                type=cursor.fetchall()[0][0],
                nullable=True,
                name=column.name,
            )

        if pg_typename == 'numeric' and column.precision is not None and column.scale is not None:
            return models.ReturnParam(
                type=f'{pg_typename}({column.precision},{column.scale})',
                nullable=True,
                name=column.name,
            )

        return models.ReturnParam(
            type=pg_typename,
            nullable=True,
            name=column.name,
        )

    def read_params(
        self,
        cursor,
        query: models.Query,
        params: list[models.QueryParam],
    ) -> tuple[
        list[models.QueryParam],
        list[models.ReturnParam],
    ]:
        stmt = f'_{uuid.uuid4().hex}'
        args = copy.deepcopy(params) if params else copy.deepcopy(query.args)
        types = [arg.type or 'unknown' for arg in args]
        types_hint = f'({", ".join(types)})' if types else ''

        cursor.execute(f'PREPARE {stmt} {types_hint} AS {query.sql}')
        cursor.execute(f"""
            SELECT
                pg_catalog.format_type(p.type_oid, NULL) AS type_name
            FROM pg_catalog.pg_prepared_statements ps
            CROSS JOIN LATERAL unnest(ps.parameter_types::oid[])
                WITH ORDINALITY AS p(type_oid, ord)
            WHERE ps.name = '{stmt}'
            ORDER BY p.ord;
        """)

        for i, row in enumerate(cursor.fetchall()):
            type = row[0]
            nullable = None

            if len(args) > i and args[i].type:
                type = args[i].type

            if len(args) > i and args[i].nullable is not None:
                nullable = args[i].nullable

            if i == len(args):
                args.append(None)

            args[i] = models.QueryParam(
                type=type,
                nullable=nullable,
            )

        cursor.execute('BEGIN')

        try:
            values = [samples.sample(arg, self.catalog) for arg in args]
            values_hint = f'({", ".join(values)})' if values else ''

            # TODO: disable all constraints: foreign keys, checks, etc...
            cursor.execute(f'EXECUTE {stmt}{values_hint}')
            description = cursor.description or []

            cursor.execute(
                'SELECT pg_catalog.format_type(oid, NULL) FROM unnest(%s::oid[]) AS oid',
                ([column.type_code for column in description],),
            )
            return_types = cursor.fetchall()
            returns = [
                self.guess_return_type(return_type[0], cursor, column)
                for return_type, column in zip(return_types, description)
            ]
        finally:
            cursor.execute('ROLLBACK')
            cursor.execute(f'DEALLOCATE {stmt}')
        return args, returns
