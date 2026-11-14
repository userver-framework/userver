import pytest

from chaotic.front import base_model
from chaotic.front import parser as front_parser


def test_array_missing_items(simple_parse):
    with pytest.raises(front_parser.ParserError) as exc:
        simple_parse({'type': 'array'})
    assert exc.value.infile_path == '/definitions/type/items'
    assert exc.value.msg == '"items" is missing'


def test_int_array(simple_parse):
    simple_parse({'type': 'array', 'items': {'type': 'integer'}})


def test_full_array(simple_parse):
    simple_parse({
        'type': 'array',
        'minItems': 1,
        'maxItems': 3,
        'items': {'type': 'integer'},
    })


def test_array_unique_items(simple_parse):
    simple_parse({
        'type': 'array',
        'uniqueItems': True,
        'items': {'type': 'integer'},
    })


def test_int_array_array_array(simple_parse):
    simple_parse({
        'type': 'array',
        'items': {
            'type': 'array',
            'items': {'type': 'array', 'items': {'type': 'integer'}},
        },
    })


@pytest.mark.parametrize('default', [None, [], [1, 2]])
def test_array_default(simple_parse, default):
    simple_parse({'type': 'array', 'items': {'type': 'integer'}, 'default': default})


def test_array_non_list_default(simple_parse):
    with pytest.raises(front_parser.ParserError) as exc:
        simple_parse({'type': 'array', 'items': {'type': 'integer'}, 'default': 1})
    assert exc.value.infile_path == '/definitions/type/default'


@pytest.mark.parametrize(
    'schema',
    [
        {'type': 'object', 'properties': {}, 'additionalProperties': False, 'default': {}},
        {'allOf': [{'type': 'object', 'properties': {}, 'additionalProperties': False}], 'default': {}},
        {'oneOf': [{'type': 'integer'}], 'default': 1},
        {'$ref': '#/definitions/other', 'default': []},
    ],
)
def test_unsupported_default(simple_parse, schema):
    with pytest.raises(front_parser.ParserError) as exc:
        simple_parse(schema)
    assert exc.value.infile_path == '/definitions/type'
    assert exc.value.msg == base_model.UNSUPPORTED_DEFAULT_MSG
