import collections

import pytest

from chaotic.front import ref_resolver
from chaotic.front import types
from chaotic.front.parser import ParserConfig
from chaotic.front.parser import ParserError
from chaotic.front.parser import SchemaParser
from chaotic.front.types import Array
from chaotic.front.types import Integer
from chaotic.front.types import Ref


def test_ref_ok(schema_parser):
    parser = schema_parser

    parser.parse_schema('/definitions/type1', {'type': 'integer'})
    parser.parse_schema('/definitions/type2', {'$ref': '#/definitions/type1'})

    parsed = parser.parsed_schemas().schemas
    assert parsed == {
        'vfull#/definitions/type1': Integer(),
        'vfull#/definitions/type2': Ref(
            ref='vfull#/definitions/type1',
            indirect=False,
            self_ref=False,
        ),
    }


def test_ref_from_items_ok(schema_parser):
    parser = schema_parser

    parser.parse_schema('/definitions/type1', {'type': 'integer'})
    parser.parse_schema(
        '/definitions/type2',
        {'type': 'array', 'items': {'$ref': '#/definitions/type1'}},
    )

    parsed = parser.parsed_schemas().schemas
    assert parsed == {
        'vfull#/definitions/type1': Integer(),
        'vfull#/definitions/type2': Array(
            items=Ref(
                ref='vfull#/definitions/type1',
                indirect=False,
                self_ref=False,
            ),
        ),
    }


def test_ref_invalid(schema_parser):
    parser = schema_parser

    parser.parse_schema('/definitions/type1', {'type': 'integer'})
    parser.parse_schema(
        '/definitions/type2',
        {'$ref': '#/definitions/other_type'},
    )
    rr = ref_resolver.RefResolver()

    with pytest.raises(Exception) as exc:
        rr.sort_schemas(parser.parsed_schemas())

    assert str(exc.value) == (
        '$ref to unknown type "vfull#/definitions/other_type", '
        'known refs:\n- vfull#/definitions/type1\n'
        '- vfull#/definitions/type2'
    )


def test_extra_fields(simple_parse):
    with pytest.raises(ParserError) as exc:
        simple_parse({'$ref': '123', 'field': 1})
    assert exc.value.infile_path == '/definitions/type'
    assert exc.value.msg == "Unknown field(s) ['field']"


def test_sibling_file(schema_parser):
    config = ParserConfig(erase_prefix='')
    schemas = []
    parser = schema_parser
    parser.parse_schema('/definitions/type1', {'type': 'integer'})
    schemas.append(parser.parsed_schemas())

    parser = SchemaParser(
        config=config,
        full_filepath='full2',
        full_vfilepath='vfull2',
    )
    parser.parse_schema(
        '/definitions/type2',
        {'$ref': 'vfull#/definitions/type1'},
    )
    schemas.append(parser.parsed_schemas())
    rr = ref_resolver.RefResolver()
    parsed_schemas = rr.sort_schemas(types.ParsedSchemas.merge(schemas))

    var = Integer(
        type='integer',
        default=None,
        nullable=False,
        minimum=None,
        maximum=None,
        enum=None,
        format=None,
    )
    assert parsed_schemas.schemas == {
        'vfull#/definitions/type1': var,
        'vfull2#/definitions/type2': Ref(
            ref='vfull#/definitions/type1',
            schema=var,
            indirect=False,
            self_ref=False,
        ),
    }


def test_forward_reference(schema_parser):
    parser = schema_parser
    parser.parse_schema('/definitions/type1', {'$ref': '#/definitions/type2'})
    parser.parse_schema('/definitions/type2', {'type': 'integer'})
    parser.parse_schema('/definitions/type3', {'$ref': '#/definitions/type4'})
    parser.parse_schema('/definitions/type4', {'$ref': '#/definitions/type2'})

    rr = ref_resolver.RefResolver()
    parsed_schemas = rr.sort_schemas(parser.parsed_schemas())

    var = Integer(
        type='integer',
        default=None,
        nullable=False,
        minimum=None,
        maximum=None,
        enum=None,
        format=None,
    )
    assert parsed_schemas.schemas == collections.OrderedDict({
        'vfull#/definitions/type2': var,
        'vfull#/definitions/type1': Ref(
            ref='vfull#/definitions/type2',
            schema=var,
            indirect=False,
            self_ref=False,
        ),
        'vfull#/definitions/type4': Ref(
            ref='vfull#/definitions/type2',
            schema=var,
            indirect=False,
            self_ref=False,
        ),
        'vfull#/definitions/type3': Ref(
            ref='vfull#/definitions/type4',
            schema=var,
            indirect=False,
            self_ref=False,
        ),
    })


def test_cycle(schema_parser):
    parser = schema_parser
    parser.parse_schema('/definitions/type1', {'$ref': '#/definitions/type2'})
    parser.parse_schema('/definitions/type2', {'$ref': '#/definitions/type1'})

    rr = ref_resolver.RefResolver()

    with pytest.raises(ref_resolver.ResolverError) as exc:
        rr.sort_schemas(parser.parsed_schemas())

    assert str(exc.value) == '$ref cycle: vfull#/definitions/type1, vfull#/definitions/type2'


def test_self_ref(schema_parser):
    parser = schema_parser
    parser.parse_schema('/definitions/type1', {'$ref': '#/definitions/type1'})

    rr = ref_resolver.RefResolver()

    with pytest.raises(ref_resolver.ResolverError) as exc:
        rr.sort_schemas(parser.parsed_schemas())

    assert str(exc.value) == '$ref cycle: vfull#/definitions/type1'


def test_no_fragment():
    config = ParserConfig(erase_prefix='')
    parser = SchemaParser(
        config=config,
        full_filepath='full',
        full_vfilepath='vfull',
    )
    with pytest.raises(ParserError) as exc_info:
        parser.parse_schema('/definitions/type1', {'$ref': '/definitions/type2'})
    assert exc_info.value.msg == 'Error in $ref (/definitions/type2): there should be exactly one "#" inside'
