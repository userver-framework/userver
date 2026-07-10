import os
import tempfile

import psycopg2

from testsuite.databases.pgsql import service
from testsuite.environment.service import ScriptService

from sqldto.shared.utils import logging

logger = logging.logger

EXTERNAL_DSN_ENV = 'POSTGRES_TEST_DSN'


class PgRunner:
    def __init__(self, dsn: str) -> None:
        self._dsn = dsn

    def connect(self):
        return psycopg2.connect(self._dsn)

    def start(self, verbose=0) -> None:
        pass

    def stop(self, verbose=0) -> None:
        pass

    def __enter__(self):
        self.start()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb) -> None:
        self.stop()


class ServicePgRunner(PgRunner):
    def __init__(self, service_name='') -> None:
        pg_settings = service.get_service_settings()
        conninfo = pg_settings.get_conninfo().replace(dbname='postgres')
        super().__init__(conninfo.get_dsn())
        self._pg_settings = pg_settings
        self._service_name = service_name
        self._working_dir: tempfile.TemporaryDirectory | None = None
        self._pg_service: ScriptService | None = None

    def start(self, verbose=0) -> None:
        self._working_dir = tempfile.TemporaryDirectory()
        self._pg_service = service.create_pgsql_service(
            service_name=self._service_name,
            working_dir=self._working_dir.name,
            settings=self._pg_settings,
        )
        self._pg_service.ensure_started(verbose=verbose)
        logger.debug('PostgreSQL URI: %s', self._pg_settings.get_conninfo().get_uri())

    def stop(self, verbose=0) -> None:
        if self._pg_service:
            self._pg_service.stop(verbose=verbose)

        if self._working_dir:
            self._working_dir.cleanup()


def create_pg_runner(service_name='') -> PgRunner:
    if external_dsn := os.environ.get(EXTERNAL_DSN_ENV):
        return PgRunner(external_dsn)
    return ServicePgRunner(service_name)
