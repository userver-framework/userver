from hashlib import sha256
import json
import pathlib

from sqldto.shared.loaders import migrations
from sqldto.shared.loaders import queries
from sqldto.shared.types import errors
from sqldto.shared.types import models


class DumpLoader:
    def __init__(
        self,
        dump_dir: pathlib.Path,
        migrations_dir: pathlib.Path,
        queries_dir: pathlib.Path | None,
        query_files: list[str] | None,
        dump_command: str,
    ) -> None:
        self.dump_file: pathlib.Path = dump_dir / 'schema.dto.json'
        self.migrations_dir: pathlib.Path = migrations_dir
        self.queries_dir: pathlib.Path | None = queries_dir
        self.query_files: list[str] | None = query_files
        self.dump_command: str = dump_command

        self.migrations: list[models.Migration] = []
        self.queries: list[models.Query] = []

    def load(self) -> None:
        self.migrations = migrations.load(self.migrations_dir)

        if self.queries_dir is not None and self.query_files is not None:
            self.queries = queries.load(self.queries_dir, self.query_files)

    def hashsum(self) -> str:
        source = {
            'migrations': [
                {
                    'name': migration.path.name,
                    'sql': migration.sql,
                }
                for migration in self.migrations
            ],
            'queries': [
                {
                    'name': query.path.name,
                    'sql': query.sql,
                }
                for query in self.queries
            ],
        }
        return sha256(json.dumps(source, sort_keys=True).encode()).hexdigest()

    def load_cached(self) -> models.Schema | None:
        hashsum = self.hashsum()

        if self.dump_file.is_file():
            dump = json.loads(self.dump_file.read_text())
            schema = models.Schema.from_dict(dump)
            if schema.hashsum == hashsum:
                return schema

        return None

    def load_or_raise(self) -> models.Schema:
        if not self.dump_file.is_file():
            raise errors.ClientError(f"""
                DTO schema dump not found: {self.dump_file}.
                Generate it manually: {self.dump_command}
            """)

        if schema := self.load_cached():
            return schema

        raise errors.ClientError(f"""
            DTO schema dump is outdated.
            Regenerate it manually: {self.dump_command}
        """)
