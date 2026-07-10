import logging
import sys

from sqldto.dumper.postgres import catalog_analyzer
from sqldto.dumper.postgres import generator
from sqldto.dumper.postgres import queries_analyzer
from sqldto.dumper.postgres import runner
from sqldto.shared import cli
from sqldto.shared.types import errors
from sqldto.shared.types import models
from sqldto.shared.utils import rendering


def fetch(context: rendering.Context) -> models.Schema:
    with runner.create_pg_runner() as pg, pg.connect() as conn, conn.cursor() as cursor:
        pg_catalog_analyzer = catalog_analyzer.PgCatalogAnalyzer()
        pg_catalog_analyzer.apply_migrations(cursor, context.fetcher.migrations)
        catalog = pg_catalog_analyzer.catalog

        conn.commit()

        pg_query_analyzer = queries_analyzer.PgQueryAnalyzer(catalog)
        queries = pg_query_analyzer.read_schemas(cursor, context.fetcher.queries)
        return models.Schema(
            hashsum=context.fetcher.hashsum(),
            catalog=catalog,
            queries=queries,
        )


def plan(context: rendering.Context) -> list[rendering.ToGenerate]:
    if schema := context.fetcher.load_cached():
        context.schemas = schema
    else:
        context.schemas = fetch(context)

    migrations = generator.PgMigrationGenerator(context).generate()

    for pg_migration in migrations:
        new_migration = models.Migration(
            path=pg_migration.output_file,
            version=context.schemas.migrations[-1].next_version(),
            sql=pg_migration.content,
        )
        context.fetcher.migrations.append(new_migration)
        context.schemas = fetch(context)

    dumps = generator.PgDumpGenerator(context).generate()
    return [*migrations, *dumps]


def dump(context: rendering.Context) -> None:
    for to_generate in plan(context):
        to_generate.render()


def main() -> int:
    logging.basicConfig(level=logging.INFO, stream=sys.stdout)

    try:
        context = cli.make_context(cli.parse_args())
        dump(context)
        return 0

    except errors.UnexpectedError:
        raise

    except errors.BaseError as e:
        print('#' * 80)
        print(e)
        print('#' * 80)
        return 1


if __name__ == '__main__':
    sys.exit(main())
