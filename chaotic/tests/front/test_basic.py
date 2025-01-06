from chaotic.front import ref_resolver
from chaotic.front.parser import ParserConfig
from chaotic.front.parser import ParserError
from chaotic.front.parser import SchemaParser


def test_generic_error(simple_parse):
    try:
        simple_parse({'type': 'integer', 'unknown_field': '1'})
        assert False
    except ParserError as exc:
        assert exc.full_filepath == 'full'
        assert exc.schema_type == 'jsonschema'


def test_unknown_field(simple_parse):
    try:
        simple_parse({'type': 'integer', 'unknown_field': '1'})
        assert False
    except ParserError as exc:
        assert exc.infile_path == '/definitions/type/unknown_field'
        assert exc.msg == (
            'Unknown field: "unknown_field", known fields: '
            '["default", "enum", "exclusiveMaximum", "exclusiveMinimum", '
            '"format", "maximum", "minimum", "nullable", "type"]'
        )


def test_duplicate_path(simple_parse):
    try:
        config = ParserConfig(erase_prefix='')
        parser = SchemaParser(
            config=config, full_filepath='full', full_vfilepath='vfull',
        )
        parser.parse_schema('/definitions/type', {'type': 'integer'})
        parser.parse_schema('/definitions/type', {'type': 'number'})
        assert False
    except ParserError as exc:
        assert exc.infile_path == '/definitions/type'
        assert exc.msg == 'Duplicate path: vfull#/definitions/type'


def test_x_unknown_field(simple_parse):
    simple_parse({'type': 'integer', 'x-unknown_field': '1'})


def test_x_known_field(simple_parse):
    simple_parse({'type': 'integer', 'x-go-xxxx': '1'})


def test_wrong_field_type(simple_parse):
    try:
        simple_parse({'type': 'intxxxx'})
        assert False
    except ParserError as exc:
        assert exc.infile_path == '/definitions/type/type'
        assert exc.msg == 'Unknown type "intxxxx"'


def test_no_schema_type(simple_parse):
    try:
        simple_parse({'enum': [1, 2, 3]})
        assert False
    except ParserError as exc:
        assert exc.infile_path == '/definitions/type'
        assert exc.msg == '"type" is missing'


def test_schema_type_string(simple_parse):
    simple_parse({'type': 'string'})


def test_ref(simple_parse):
    try:
        config = ParserConfig(erase_prefix='')
        parser = SchemaParser(
            config=config, full_filepath='full', full_vfilepath='vfull',
        )
        parser.parse_schema(
            '/definitions/type1', {'$ref': '#/definitions/type'},
        )
        rr = ref_resolver.RefResolver()
        rr.sort_schemas(parser.parsed_schemas())
        assert False
    except Exception as exc:  # pylint: disable=broad-exception-caught
        assert str(exc) == (
            '$ref to unknown type "vfull#/definitions/type", known refs:\n'
            '- vfull#/definitions/type1'
        )
