import pathlib
import re

from sqldto.shared.types import errors
from sqldto.shared.types import models
from sqldto.shared.utils import logging

MIGRATION_PATTERN = re.compile(r'^V(\d+)__.+\.sql$')

logger = logging.logger


def load(migrations_dir: pathlib.Path) -> list[models.Migration]:
    if not migrations_dir.is_dir():
        raise errors.ClientError(
            f"""
            Can't load migrations from directory: {migrations_dir}.
            Directory does not exist or is not a directory.
            """
        )

    logger.debug('Loading migrations from %s', migrations_dir)

    paths = sorted(migrations_dir.glob('*.sql'), key=lambda p: p.name)

    if not paths:
        logger.warning('Not found any migration! Expected *.sql files')
        return []

    for path in paths:
        if not MIGRATION_PATTERN.match(path.name):
            raise errors.ClientError(
                f"""
                Found migration file with unexpected name: {path.name}
                Expected pattern: V<version>__<name>.sql.
                """
            )

    migrations = [
        models.Migration(
            path=path,
            version=matched.group(1),
            sql=path.read_text(encoding='utf-8'),
        )
        for path in paths
        if (matched := MIGRATION_PATTERN.match(path.name))
    ]

    return sorted(migrations, key=lambda v: int(v.version))
