import pytest

from chaotic.front import ref_resolver
from chaotic.front.parser import ParserError


def test_generic_error(simple_parse):
    with pytest.raises(ParserError) as exc:
        simple_parse({'type': 'integer', 'unknown_field': '1'})
    assert exc.value.full_filepath == 'full'
    assert exc.value.schema_type == 'jsonschema'


def test_unknown_field(simple_parse):
    with pytest.raises(ParserError) as exc:
        simple_parse({'type': 'integer', 'unknown_field': '1'})
    assert exc.value.infile_path == '/definitions/type/unknown_field'
    assert exc.value.msg == (
        'Unknown field: "unknown_field", known fields: '
        '["default", "enum", "exclusiveMaximum", "exclusiveMinimum", '
        '"format", "maximum", "minimum", "nullable", "type"]'
    )


def test_duplicate_path(schema_parser):
    with pytest.raises(ParserError) as exc:
        parser = schema_parser
        parser.parse_schema('/definitions/type', {'type': 'integer'})
        parser.parse_schema('/definitions/type', {'type': 'number'})
    assert exc.value.infile_path == '/definitions/type'
    assert exc.value.msg == 'Duplicate path: vfull#/definitions/type'


def test_x_unknown_field(simple_parse):
    simple_parse({'type': 'integer', 'x-unknown_field': '1'})


def test_x_known_field(simple_parse):
    simple_parse({'type': 'integer', 'x-go-xxxx': '1'})


def test_wrong_field_type(simple_parse):
    with pytest.raises(ParserError) as exc:
        simple_parse({'type': 'intxxxx'})
    assert exc.value.infile_path == '/definitions/type/type'
    assert exc.value.msg == 'Unknown type "intxxxx"'


def test_no_schema_type(simple_parse):
    with pytest.raises(ParserError) as exc:
        simple_parse({'enum': [1, 2, 3]})
    assert exc.value.infile_path == '/definitions/type'
    assert exc.value.msg == '"type" is missing'


def test_schema_type_string(simple_parse):
    simple_parse({'type': 'string'})


def test_title(simple_parse):
    simple_parse({'type': 'string', 'title': 'something'})


def test_ref(schema_parser):
    with pytest.raises(Exception) as exc_info:
        parser = schema_parser
        parser.parse_schema(
            '/definitions/type1',
            {'$ref': '#/definitions/type'},
        )
        rr = ref_resolver.RefResolver()
        rr.sort_schemas(parser.parsed_schemas())

    assert str(exc_info.value) == (
        '$ref to unknown type "vfull#/definitions/type", known refs:\n- vfull#/definitions/type1'
    )
