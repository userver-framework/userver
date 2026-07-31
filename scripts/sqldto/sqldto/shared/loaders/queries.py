import pathlib
import re

import sqlparse

from sqldto.shared.types import errors
from sqldto.shared.types import models
from sqldto.shared.utils import logging

QUERY_PATTERN = re.compile(r'^.+\.sql$')
NO_CODEGEN_PATTERN = re.compile(r'@no-dto\b')
CARDINALITY_PATTERN = re.compile(r'@cardinality\s*:\s*(.+)')
ARG_PATTERN = re.compile(r'@arg(\d+)\s*:\s*(.+)')
ANY_PATTERN = re.compile(r'@[\w-]+')

logger = logging.logger


def parse_comments(sql: str) -> list[str]:
    parsed = sqlparse.parse(sql)
    comments = []
    for stmt in parsed:
        for token in stmt.flatten():
            if token.ttype in sqlparse.tokens.Comment:
                comments.append(str(token).strip())
    return comments


def parse_directives(query: models.Query) -> None:
    ALL_PATTERNS = [NO_CODEGEN_PATTERN, CARDINALITY_PATTERN, ARG_PATTERN]

    for comment in parse_comments(query.sql):
        directives = ANY_PATTERN.findall(comment)
        if not directives:
            continue

        if len(directives) > 1:
            raise errors.ClientError(
                f"""
                Found multiple directives in query: {query.path.name}.
                Please split them into different lines.
                Errors in comment:
                {comment}
                """
            )

        if not any(pattern.search(comment) for pattern in ALL_PATTERNS):
            raise errors.ClientError(
                f"""
                Found unknown directive in query: {query.path.name}.
                Please use one of the following directives:
                - @no-dto
                - @cardinality: <cardinality>
                - @arg<N>: <type>
                Errors in comment:
                {comment}
                """
            )


def parse_no_codegen(query: models.Query) -> bool:
    return any(NO_CODEGEN_PATTERN.search(comment) for comment in parse_comments(query.sql))


def parse_cardinality(query: models.Query) -> models.QueryCardinality | None:
    for comment in parse_comments(query.sql):
        match = CARDINALITY_PATTERN.search(comment)
        if not match:
            continue

        cardinality = match.group(1).strip().lower()
        if cardinality in [item.value for item in models.QueryCardinality]:
            return models.QueryCardinality(cardinality)

        raise errors.ClientError(
            f"""
                Found unknown cardinality in query: {query.path.name}.
                Allowed values: {', '.join([item.value for item in models.QueryCardinality])}
                Cardinality in comment:
                {comment}
                """
        )

    return None


def parse_args(query: models.Query) -> list[models.QueryParam]:
    overrides: dict[int, str] = {}
    for comment in parse_comments(query.sql):
        match = ARG_PATTERN.search(comment)
        if not match:
            continue
        overrides[int(match.group(1))] = match.group(2).strip().lower()

    max_index = max([0] + list(overrides.keys()))
    return [
        models.QueryParam(
            type=overrides.get(i + 1),
            nullable=None,
        )
        for i in range(max_index)
    ]


def load_query(queries_dir: pathlib.Path, query_file: pathlib.Path) -> models.Query:
    sql = query_file.read_text(encoding='utf-8')
    query = models.Query(
        path=query_file,
        sql=sql,
        source=str(query_file.relative_to(queries_dir)),
        no_codegen=False,
        args=[],
        returns=[],
        cardinality=None,
    )
    parse_directives(query)
    query.no_codegen = parse_no_codegen(query)
    query.cardinality = parse_cardinality(query)
    query.args = parse_args(query)
    return query


def load(queries_dir: pathlib.Path, query_files: list[str]) -> list[models.Query]:
    if not queries_dir.is_dir():
        raise errors.ClientError(
            f"""
            Can't load queries from directory: {queries_dir}.
            Directory does not exist or is not a directory.
            """
        )

    logger.debug('Loading queries from %s', queries_dir)

    paths = [pathlib.Path(path) for path in sorted(query_files)]
    queries = [load_query(queries_dir, path) for path in paths]
    return sorted(queries, key=lambda q: q.path.name)
