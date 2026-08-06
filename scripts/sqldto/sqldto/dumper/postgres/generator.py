import json

import jinja2

from sqldto.shared.types import models
from sqldto.shared.utils import logging
from sqldto.shared.utils import rendering

logger = logging.logger


class PgMigrationGenerator:
    def __init__(self, context: rendering.Context) -> None:
        self.context = context
        self.loader = jinja2.PackageLoader(__package__, 'templates')

    def generate(self) -> list[rendering.ToGenerate]:
        if not self.context.migrations:
            return []

        if not self.context.migrations_output_dir:
            return []

        types_to_generate: list[models.StructType] = []

        for table in self.context.schemas.catalog.tables.values():
            if table.type_name not in self.context.schemas.catalog.types:
                types_to_generate.append(models.table_to_type(table))
                continue

            if models.table_like_type(table, self.context.schemas.catalog.types[table.type_name]):
                continue

            logger.warning(
                'Conflict type %s for table %s, skip it',
                table.type_name,
                table.db_name,
            )

        if not types_to_generate:
            return []

        version = self.context.migrations[-1].next_version()

        to_generate = rendering.ToGenerate(
            loader=self.loader,
            template_name='VX__codegen_migration.sql.jinja',
            output_file=self.context.migrations_output_dir / f'V{version}__codegen_migration.sql',
            context={
                'types': types_to_generate,
            },
        )
        return [to_generate]


class PgDumpGenerator:
    def __init__(self, context: rendering.Context) -> None:
        self.context = context

    def generate(self) -> list[rendering.ToGenerate]:
        if self.context.fetcher.load_cached() is not None:
            logger.debug('Dump is already up to date, do nothing')
            return []

        content = json.dumps(
            self.context.schemas.to_dict(),
            sort_keys=True,
            indent=2,
            separators=(',', ': '),
            ensure_ascii=False,
        )
        to_generate = rendering.ToGenerate(
            output_file=self.context.fetcher.dump_file,
            content=content + '\n',
        )
        return [to_generate]
