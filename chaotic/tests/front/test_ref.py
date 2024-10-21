import collections

from chaotic.front import ref_resolver
from chaotic.front import types
from chaotic.front.parser import ParserConfig
from chaotic.front.parser import ParserError
from chaotic.front.parser import SchemaParser
from chaotic.front.types import Array
from chaotic.front.types import Integer
from chaotic.front.types import Ref


def test_ref_ok():
    config = ParserConfig(erase_prefix='')
    parser = SchemaParser(
        config=config, full_filepath='full', full_vfilepath='vfull',
    )

    parser.parse_schema('/definitions/type1', {'type': 'integer'})
    parser.parse_schema('/definitions/type2', {'$ref': '#/definitions/type1'})

    parsed = parser.parsed_schemas().schemas
    assert parsed == {
        'vfull#/definitions/type1': Integer(),
        'vfull#/definitions/type2': Ref(
            ref='vfull#/definitions/type1', indirect=False, self_ref=False,
        ),
    }


def test_ref_from_items_ok():
    config = ParserConfig(erase_prefix='')
    parser = SchemaParser(
        config=config, full_filepath='full', full_vfilepath='vfull',
    )

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
                ref='vfull#/definitions/type1', indirect=False, self_ref=False,
            ),
        ),
    }


def test_ref_invalid():
    config = ParserConfig(erase_prefix='')
    parser = SchemaParser(
        config=config, full_filepath='full', full_vfilepath='vfull',
    )

    try:
        parser.parse_schema('/definitions/type1', {'type': 'integer'})
        parser.parse_schema(
            '/definitions/type2', {'$ref': '#/definitions/other_type'},
        )
        rr = ref_resolver.RefResolver()
        rr.sort_schemas(parser.parsed_schemas())
        assert False
    except Exception as exc:  # pylint: disable=broad-exception-caught
        assert str(exc) == (
            '$ref to unknown type "vfull#/definitions/other_type", '
            'known refs:\n- vfull#/definitions/type1\n'
            '- vfull#/definitions/type2'
        )


def test_extra_fields(simple_parse):
    try:
        simple_parse({'$ref': '123', 'field': 1})
        assert False
    except ParserError as exc:
        assert exc.infile_path == '/definitions/type'
        assert exc.msg == "Unknown field(s) ['field']"


def test_sibling_file():
    config = ParserConfig(erase_prefix='')
    schemas = []
    parser = SchemaParser(
        config=config, full_filepath='full', full_vfilepath='vfull',
    )
    parser.parse_schema('/definitions/type1', {'type': 'integer'})
    schemas.append(parser.parsed_schemas())

    parser = SchemaParser(
        config=config, full_filepath='full2', full_vfilepath='vfull2',
    )
    parser.parse_schema(
        '/definitions/type2', {'$ref': 'vfull#/definitions/type1'},
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


def test_forward_reference():
    config = ParserConfig(erase_prefix='')
    parser = SchemaParser(
        config=config, full_filepath='full', full_vfilepath='vfull',
    )
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


def test_cycle():
    config = ParserConfig(erase_prefix='')
    parser = SchemaParser(
        config=config, full_filepath='full', full_vfilepath='vfull',
    )
    parser.parse_schema('/definitions/type1', {'$ref': '#/definitions/type2'})
    parser.parse_schema('/definitions/type2', {'$ref': '#/definitions/type1'})

    rr = ref_resolver.RefResolver()
    try:
        rr.sort_schemas(parser.parsed_schemas())
    except ref_resolver.ResolverError as exc:
        assert (
            str(exc)
            == '$ref cycle: vfull#/definitions/type1, vfull#/definitions/type2'
        )
    else:
        assert False
