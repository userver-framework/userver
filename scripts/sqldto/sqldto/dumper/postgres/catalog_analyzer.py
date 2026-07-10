from sqldto.shared.types import models


class PgCatalogAnalyzer:
    def __init__(self) -> None:
        self.catalog: models.Catalog = models.Catalog(tables={}, types={}, enums={})

    def apply_migrations(self, cursor, migrations: list[models.Migration]) -> None:
        for migration in migrations:
            self.apply_migration(cursor, migration)

    def apply_migration(self, cursor, migration: models.Migration) -> None:
        cursor.execute(migration.sql)

        for new_table in self.read_tables(cursor=cursor).values():
            old_table = self.catalog.tables.get(new_table.db_name)

            if not old_table:
                new_table.version = 0

            elif old_table != new_table:
                new_table.version = old_table.version + 1

            else:
                new_table.version = old_table.version

            self.catalog.tables[new_table.db_name] = new_table

        self.catalog.types = self.read_types(cursor=cursor)
        self.catalog.enums = self.read_enums(cursor=cursor)

    @staticmethod
    def read_tables(cursor) -> dict[str, models.Table]:
        cursor.execute("""
            SELECT
                nspname AS namespace_name,
                relname AS table_name,
                attname AS column_name,
                format_type(atttypid, atttypmod) AS column_type,
                attnotnull AS column_not_nullable
            FROM pg_catalog.pg_attribute a
            JOIN pg_catalog.pg_class c ON c.oid = a.attrelid
            JOIN pg_catalog.pg_namespace n ON n.oid = c.relnamespace
            WHERE a.attnum > 0 AND
            NOT a.attisdropped AND
            n.nspname NOT IN ('pg_catalog', 'information_schema') AND
            (
                c.relkind = 'p' OR
                (c.relkind = 'r' AND NOT c.relispartition) OR
                c.relkind IN ('v', 'm')
            )
            ORDER BY n.nspname, c.relname, a.attnum
        """)
        rows = cursor.fetchall()

        tables: dict[str, models.Table] = {}
        for row in rows:
            namespace_name = row[0]
            table_name = row[1]
            column_name = row[2]
            column_type = row[3]
            column_nullable = not row[4]

            column = models.Column(
                name=column_name,
                type=column_type,
                nullable=column_nullable,
            )

            table = models.Table(
                namespace=namespace_name,
                name=table_name,
                columns=[column],
                version=0,
            )

            if table.db_name in tables:
                tables[table.db_name].columns.append(column)
            else:
                tables[table.db_name] = table

        return tables

    @staticmethod
    def read_types(cursor) -> dict[str, models.StructType]:
        cursor.execute("""
            SELECT
                n.nspname AS namespace_name,
                t.typname AS type_name,
                a.attname AS field_name,
                pg_catalog.format_type(a.atttypid, a.atttypmod) AS field_type
            FROM pg_catalog.pg_type t
            JOIN pg_catalog.pg_namespace n ON n.oid = t.typnamespace
            JOIN pg_catalog.pg_class c ON c.oid = t.typrelid
            JOIN pg_catalog.pg_attribute a ON a.attrelid = c.oid
            WHERE t.typtype = 'c'
            AND c.relkind = 'c'
            AND a.attnum > 0
            AND NOT a.attisdropped
            AND n.nspname NOT IN ('pg_catalog', 'information_schema')
            ORDER BY n.nspname, c.relname, a.attnum
        """)
        rows = cursor.fetchall()

        types: dict[str, models.StructType] = {}
        for row in rows:
            namespace_name = row[0]
            type_name = row[1]
            field_name = row[2]
            field_type = row[3]

            field = models.Column(
                name=field_name,
                type=field_type,
                nullable=True,  # all fields are nullable
            )

            type_ = models.StructType(
                namespace=namespace_name,
                name=type_name,
                fields=[field],
            )

            if type_.db_name in types:
                types[type_.db_name].fields.append(field)
            else:
                types[type_.db_name] = type_

        return types

    @staticmethod
    def read_enums(cursor) -> dict[str, models.DbEnum]:
        cursor.execute("""
        SELECT
            n.nspname AS namespace_name,
            t.typname AS type_name,
            e.enumlabel AS enum_value
        FROM pg_catalog.pg_enum e
        JOIN pg_catalog.pg_type t ON t.oid = e.enumtypid
        JOIN pg_catalog.pg_namespace n ON n.oid = t.typnamespace
        WHERE n.nspname NOT IN ('pg_catalog', 'information_schema')
        ORDER BY n.nspname, t.typname, e.enumsortorder
        """)
        rows = cursor.fetchall()

        enums: dict[str, models.DbEnum] = {}
        for row in rows:
            namespace_name = row[0]
            type_name = row[1]
            enum_value = row[2]

            enum = models.DbEnum(
                namespace=namespace_name,
                name=type_name,
                values=[enum_value],
            )

            if enum.db_name in enums:
                enums[enum.db_name].values.append(enum_value)
            else:
                enums[enum.db_name] = enum

        return enums
