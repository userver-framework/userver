import pytest

from chaotic.front import parser as front_parser
from chaotic.front import types as front_types


@pytest.fixture(name='parse_after_refs')
def _parse_after_refs(schema_parser, clear_source_location):
    def func(input_: dict):
        parser = schema_parser
        parser.parse_schema(
            '/definitions/type1',
            {
                'type': 'object',
                'properties': {
                    'foo': {'type': 'string'},
                    'bar': {'type': 'number'},
                },
                'additionalProperties': False,
            },
        )
        parser.parse_schema(
            '/definitions/type2',
            {
                'type': 'object',
                'properties': {
                    'foo': {'type': 'string'},
                    'bar': {'type': 'boolean'},
                },
                'additionalProperties': False,
            },
        )
        parser.parse_schema(
            '/definitions/type3',
            {
                'type': 'object',
                'properties': {
                    'version': {'type': 'integer'},
                    'prop': {'type': 'string'},
                },
                'additionalProperties': False,
            },
        )
        parser.parse_schema(
            '/definitions/type4',
            {
                'type': 'object',
                'properties': {
                    'version': {'type': 'integer'},
                    'prop': {'type': 'integer'},
                },
                'additionalProperties': False,
            },
        )
        parser.parse_schema('/definitions/type_int', {'type': 'integer'})
        parser.parse_schema(
            '/definitions/wrong_type',
            {
                'type': 'object',
                'properties': {'bar': {'type': 'integer'}},
                'additionalProperties': False,
            },
        )
        parser.parse_schema('/definitions/type', input_)

        for schema in parser.parsed_schemas().schemas.values():
            schema.visit_children(clear_source_location)
            clear_source_location(schema, None)

        return parser.parsed_schemas().schemas

    return func


REFS = {
    'vfull#/definitions/type1': front_types.SchemaObject(
        properties={'foo': front_types.String(), 'bar': front_types.Number()},
        additionalProperties=False,
    ),
    'vfull#/definitions/type2': front_types.SchemaObject(
        properties={'foo': front_types.String(), 'bar': front_types.Boolean()},
        additionalProperties=False,
    ),
    'vfull#/definitions/type3': front_types.SchemaObject(
        properties={'version': front_types.Integer(), 'prop': front_types.String()},
        additionalProperties=False,
    ),
    'vfull#/definitions/type4': front_types.SchemaObject(
        properties={'version': front_types.Integer(), 'prop': front_types.Integer()},
        additionalProperties=False,
    ),
    'vfull#/definitions/type_int': front_types.Integer(),
    'vfull#/definitions/wrong_type': front_types.SchemaObject(
        properties={'bar': front_types.Integer()},
        additionalProperties=False,
    ),
}


def test_of_none(simple_parse):
    with pytest.raises(front_parser.ParserError) as exc:
        simple_parse({'oneOf': []})
    assert exc.value.infile_path == '/definitions/type/oneOf'
    assert exc.value.msg == 'Empty oneOf'


# stupid, but valid
def test_wo_discriminator_1(simple_parse):
    parsed = simple_parse({'oneOf': [{'type': 'integer'}]})
    assert parsed.schemas == {
        'vfull#/definitions/type': front_types.OneOfWithoutDiscriminator(oneOf=[front_types.Integer()]),
    }


def test_wo_discriminator_2(simple_parse):
    parsed = simple_parse({
        'oneOf': [
            {'type': 'integer'},
            {
                'type': 'object',
                'properties': {},
                'additionalProperties': False,
            },
        ],
    })
    assert parsed.schemas == {
        'vfull#/definitions/type': front_types.OneOfWithoutDiscriminator(
            oneOf=[
                front_types.Integer(),
                front_types.SchemaObject(properties={}, additionalProperties=False),
            ],
        ),
    }


def test_wo_discriminator_nullable(simple_parse):
    parsed = simple_parse({'oneOf': [{'type': 'integer'}], 'nullable': True})
    assert parsed.schemas['vfull#/definitions/type'].nullable


def test_wo_discriminator_nullable_wrong_type(simple_parse):
    with pytest.raises(front_parser.ParserError) as exc:
        simple_parse({'oneOf': [{'type': 'integer'}], 'nullable': 1})
    assert exc.value.infile_path == '/definitions/type/nullable'
    assert exc.value.msg == 'Boolean type is expected, 1 is found'


def test_wd_no_ref_or_object(simple_parse):
    with pytest.raises(front_parser.ParserError) as exc:
        simple_parse({
            'oneOf': [
                {'type': 'integer'},
                {
                    'type': 'object',
                    'properties': {},
                    'additionalProperties': False,
                },
            ],
            'discriminator': {'propertyName': 'foo'},
        })
    assert exc.value.infile_path == '/definitions/type/oneOf/0'
    assert exc.value.msg == 'Not a $ref in oneOf with discriminator'


def test_wd_wrong_property(simple_parse):
    with pytest.raises(front_parser.ParserError) as exc:
        simple_parse({
            'oneOf': [
                {
                    'type': 'object',
                    'properties': {},
                    'additionalProperties': False,
                },
            ],
            'discriminator': {'propertyName': 'foo'},
        })
    assert exc.value.infile_path == '/definitions/type/oneOf/0'
    assert exc.value.msg == 'Not a $ref in oneOf with discriminator'


def test_wd_wrong_property2(parse_after_refs):
    with pytest.raises(front_parser.ParserError) as exc:
        parse_after_refs({
            'oneOf': [
                {'$ref': '#/definitions/type1'},
                {'$ref': '#/definitions/type2'},
                {'$ref': '#/definitions/wrong_type'},
            ],
            'discriminator': {'propertyName': 'foo'},
        })
    assert exc.value.infile_path == '/definitions/type/oneOf/2'
    assert exc.value.msg == 'No discriminator property "foo"'


def test_wd_wrong_type(parse_after_refs):
    with pytest.raises(front_parser.ParserError) as exc:
        parse_after_refs({
            'oneOf': [{'$ref': '#/definitions/type_int'}],
            'discriminator': {'propertyName': 'foo'},
        })
    assert exc.value.infile_path == '/definitions/type/oneOf/0'
    assert exc.value.msg == 'oneOf $ref to non-object (Integer)'


def test_wd_ok(parse_after_refs):
    schema = parse_after_refs({
        'oneOf': [
            {'$ref': '#/definitions/type1'},
            {'$ref': '#/definitions/type2'},
        ],
        'discriminator': {'propertyName': 'foo'},
    })
    assert schema == {
        **REFS,
        'vfull#/definitions/type': front_types.OneOfWithDiscriminator(
            oneOf=[
                front_types.Ref(
                    ref='vfull#/definitions/type1',
                    indirect=False,
                    self_ref=False,
                ),
                front_types.Ref(
                    ref='vfull#/definitions/type2',
                    indirect=False,
                    self_ref=False,
                ),
            ],
            discriminator_property='foo',
            mapping=front_types.DiscMapping(str_values=[['type1'], ['type2']], int_values=None),
        ),
    }


def test_wd_ok_with_mapping(parse_after_refs):
    schema = parse_after_refs({
        'oneOf': [
            {'$ref': '#/definitions/type1'},
            {'$ref': '#/definitions/type2'},
        ],
        'discriminator': {
            'propertyName': 'foo',
            'mapping': {
                't1': '#/definitions/type1',
                't2': '#/definitions/type2',
            },
        },
    })
    assert schema == {
        **REFS,
        'vfull#/definitions/type': front_types.OneOfWithDiscriminator(
            oneOf=[
                front_types.Ref(
                    ref='vfull#/definitions/type1',
                    indirect=False,
                    self_ref=False,
                ),
                front_types.Ref(
                    ref='vfull#/definitions/type2',
                    indirect=False,
                    self_ref=False,
                ),
            ],
            discriminator_property='foo',
            mapping=front_types.DiscMapping(str_values=[['t1'], ['t2']], int_values=None),
        ),
    }


def test_wd_ok_with_int_mapping(parse_after_refs):
    schema = parse_after_refs({
        'oneOf': [
            {'$ref': '#/definitions/type3'},
            {'$ref': '#/definitions/type4'},
        ],
        'discriminator': {
            'propertyName': 'version',
            'mapping': {
                0: '#/definitions/type3',
                1: '#/definitions/type3',
                2: '#/definitions/type4',
            },
        },
    })
    assert schema == {
        **REFS,
        'vfull#/definitions/type': front_types.OneOfWithDiscriminator(
            oneOf=[
                front_types.Ref(
                    ref='vfull#/definitions/type3',
                    indirect=False,
                    self_ref=False,
                ),
                front_types.Ref(
                    ref='vfull#/definitions/type4',
                    indirect=False,
                    self_ref=False,
                ),
            ],
            discriminator_property='version',
            mapping=front_types.DiscMapping(str_values=None, int_values=[[0, 1], [2]]),
        ),
    }


def test_wd_non_uniform_mapping(parse_after_refs):
    with pytest.raises(front_parser.ParserError) as exc:
        parse_after_refs({
            'oneOf': [
                {'$ref': '#/definitions/type3'},
                {'$ref': '#/definitions/type4'},
            ],
            'discriminator': {
                'propertyName': 'version',
                'mapping': {
                    42: '#/definitions/type3',
                    '42': '#/definitions/type4',
                },
            },
        })
    assert exc.value.infile_path == '/definitions/type/discriminator/mapping'
    assert exc.value.msg == "Not uniform mapping: use the same type for keys: dict_keys([42, '42'])"


def test_wd_ok_with_mapping_missing_ref(parse_after_refs):
    with pytest.raises(front_parser.ParserError) as exc:
        parse_after_refs({
            'oneOf': [
                {'$ref': '#/definitions/type1'},
                {'$ref': '#/definitions/type2'},
            ],
            'discriminator': {
                'propertyName': 'foo',
                'mapping': {'t2': '#/definitions/type2'},
            },
        })
    assert exc.value.infile_path == '/definitions/type/discriminator/mapping'
    assert exc.value.msg == 'Missing $ref in mapping: vfull#/definitions/type1'


def test_wd_ok_with_mapping_invalid_ref(parse_after_refs):
    with pytest.raises(front_parser.ParserError) as exc:
        parse_after_refs({
            'oneOf': [
                {'$ref': '#/definitions/type1'},
                {'$ref': '#/definitions/type2'},
            ],
            'discriminator': {
                'propertyName': 'foo',
                'mapping': {
                    't1': '#/definitions/wrong',
                    't3': '#/definitions/type1',
                    't2': '#/definitions/type2',
                },
            },
        })
    assert exc.value.infile_path == '/definitions/type/discriminator/mapping'
    assert exc.value.msg == ("$ref(s) outside of oneOf: ['vfull#/definitions/wrong']")


def test_wd_invalidtype_mapping_value(parse_after_refs):
    with pytest.raises(front_parser.ParserError) as exc:
        parse_after_refs({
            'oneOf': [
                {'$ref': '#/definitions/type1'},
                {'$ref': '#/definitions/type2'},
            ],
            'discriminator': {
                'propertyName': 'foo',
                'mapping': {'t1': 1, 't2': '#/definitions/type2'},
            },
        })
    assert exc.value.infile_path == '/definitions/type/discriminator/mapping/t1'
    assert exc.value.msg == 'String type is expected, 1 is found'


def test_wd_extra_field(simple_parse):
    with pytest.raises(front_parser.ParserError) as exc:
        simple_parse({
            'oneOf': [
                {
                    'type': 'object',
                    'properties': {},
                    'additionalProperties': False,
                },
            ],
            'discriminator': {'foo': 1, 'propertyName': 'foo'},
        })
    assert exc.value.infile_path == '/definitions/type/discriminator'
    assert exc.value.msg == ("Unknown field: \"foo\", known fields: ['mapping', 'propertyName']")


def test_wd_nullable(parse_after_refs):
    schema = parse_after_refs({
        'oneOf': [
            {'$ref': '#/definitions/type1'},
            {'$ref': '#/definitions/type2'},
        ],
        'discriminator': {'propertyName': 'foo'},
        'nullable': True,
    })
    assert schema['vfull#/definitions/type'].nullable
