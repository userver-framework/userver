import pytest

from chaotic.front import parser as front_parser


def test_array_missing_items(simple_parse):
    with pytest.raises(front_parser.ParserError) as exc:
        simple_parse({'type': 'array'})
    assert exc.value.infile_path == '/definitions/type/items'
    assert exc.value.msg == '"items" is missing'


def test_int_array(simple_parse):
    simple_parse({'type': 'array', 'items': {'type': 'integer'}})


def test_array_default(simple_parse):
    simple_parse({'type': 'array', 'items': {'type': 'integer'}, 'default': [1, 2]})
    simple_parse({'type': 'array', 'items': {'type': 'integer'}, 'default': []})


def test_array_default_must_be_array(simple_parse):
    with pytest.raises(front_parser.ParserError) as exc:
        simple_parse({'type': 'array', 'items': {'type': 'integer'}, 'default': 'invalid'})
    assert exc.value.infile_path == '/definitions/type/items/default'


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
