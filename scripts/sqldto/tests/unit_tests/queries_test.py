import pathlib

import pytest

from sqldto.shared.loaders import queries
from sqldto.shared.types import errors
from sqldto.shared.types import models


def make_query(sql: str) -> models.Query:
    return models.Query(
        path=pathlib.Path('test.sql'),
        sql=sql,
        source='test.sql',
        no_codegen=False,
        args=[],
        cardinality=None,
    )


class TestParseNoCodegen:
    def test_present(self):
        query = make_query('-- @no-dto\nSELECT 1;')
        assert queries.parse_no_codegen(query) is True

    def test_absent(self):
        query = make_query('SELECT 1;')
        assert queries.parse_no_codegen(query) is False

    def test_inline_in_comment(self):
        query = make_query('-- this query is @no-dto for a reason\nSELECT 1;')
        assert queries.parse_no_codegen(query) is True


class TestParseCardinality:
    @pytest.mark.parametrize(
        'value, expected',
        [
            ('one', models.QueryCardinality.one),
            ('many', models.QueryCardinality.many),
            ('optional', models.QueryCardinality.optional),
            ('void', models.QueryCardinality.void),
        ],
    )
    def test_valid_values(self, value, expected):
        query = make_query(f'-- @cardinality: {value}\nSELECT 1;')
        assert queries.parse_cardinality(query) == expected

    def test_case_insensitive(self):
        query = make_query('-- @cardinality: ONE\nSELECT 1;')
        assert queries.parse_cardinality(query) == models.QueryCardinality.one

    def test_extra_whitespace(self):
        query = make_query('-- @cardinality:   many  \nSELECT 1;')
        assert queries.parse_cardinality(query) == models.QueryCardinality.many

    def test_absent(self):
        query = make_query('SELECT 1;')
        assert queries.parse_cardinality(query) is None

    def test_unknown_value(self):
        query = make_query('-- @cardinality: zero\nSELECT 1;')
        with pytest.raises(errors.ClientError, match='unknown cardinality'):
            queries.parse_cardinality(query)


class TestParseArgs:
    def test_no_annotations(self):
        query = make_query('SELECT 1;')
        assert queries.parse_args(query) == []

    def test_single_arg(self):
        query = make_query('-- @arg1: text\nSELECT $1;')
        result = queries.parse_args(query)
        assert result == [
            models.QueryParam(
                type='text',
                nullable=None,
            ),
        ]

    def test_multiple_args(self):
        sql = '-- @arg1: text\n-- @arg2: integer\nSELECT $1, $2;'
        query = make_query(sql)
        result = queries.parse_args(query)
        assert result == [
            models.QueryParam(
                type='text',
                nullable=None,
            ),
            models.QueryParam(
                type='integer',
                nullable=None,
            ),
        ]

    def test_pg_type_is_lowercased(self):
        query = make_query('-- @arg1: TEXT\nSELECT $1;')
        result = queries.parse_args(query)
        assert result == [
            models.QueryParam(
                type='text',
                nullable=None,
            ),
        ]

    def test_sparse_indices(self):
        query = make_query('-- @arg3: text\nSELECT $1, $2, $3;')
        result = queries.parse_args(query)
        assert result == [
            models.QueryParam(
                type=None,
                nullable=None,
            ),
            models.QueryParam(
                type=None,
                nullable=None,
            ),
            models.QueryParam(
                type='text',
                nullable=None,
            ),
        ]

    def test_duplicate_arg(self):
        # silent override — last @argN with the same N replaces previous ones
        sql = '-- @arg1: text\n-- @arg1: integer\nSELECT $1;'
        result = queries.parse_args(make_query(sql))
        assert result == [
            models.QueryParam(
                type='integer',
                nullable=None,
            ),
        ]


class TestParseDirectives:
    def test_recognised_directives(self):
        sql = '-- @no-dto\n-- @cardinality: one\n-- @arg1: text\nSELECT $1;'
        queries.parse_directives(make_query(sql))

    def test_unknown_directive(self):
        query = make_query('-- @bogus: value\nSELECT 1;')
        with pytest.raises(errors.ClientError, match='unknown directive'):
            queries.parse_directives(query)

    def test_multiple_per_line(self):
        query = make_query('-- @no-dto @cardinality: one\nSELECT 1;')
        with pytest.raises(errors.ClientError, match='multiple directives'):
            queries.parse_directives(query)

    def test_no_annotations(self):
        queries.parse_directives(make_query('SELECT 1;'))

    # Typical user typos in argument annotations.
    @pytest.mark.parametrize(
        'bad_annotation',
        [
            '-- @arg1 text',  # missing colon
            '-- @arg1: ',  # missing type after colon
            '-- @arg: text',  # missing index
            '-- @arg1text',  # missing both ":" and space
            '-- @args1: text',  # typo in directive name (plural)
            '-- @argument1: text',  # typo in directive name (full word)
            '-- @nodto',  # typo in @no-dto (missing dash)
            '-- @no_dto',  # typo in @no-dto (underscore)
            '-- @cardinaity: one',  # typo in @cardinality
            '-- @cardinality one',  # missing colon in @cardinality
        ],
    )
    def test_user_typos(self, bad_annotation):
        query = make_query(f'{bad_annotation}\nSELECT 1;')
        with pytest.raises(errors.ClientError):
            queries.parse_directives(query)
