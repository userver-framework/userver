from chaotic.front.parser import ParserError
from chaotic.front.types import AllOf
from chaotic.front.types import SchemaObject


def test_of_none(simple_parse):
    try:
        simple_parse({'allOf': []})
        assert False
    except ParserError as exc:
        assert exc.infile_path == '/definitions/type/allOf'
        assert exc.msg == 'Empty allOf'


# stupid, but valid
def test_1_empty(simple_parse):
    parsed = simple_parse(
        {
            'allOf': [
                {
                    'type': 'object',
                    'properties': {},
                    'additionalProperties': False,
                },
            ],
        },
    )
    assert parsed.schemas == {
        'vfull#/definitions/type': AllOf(
            allOf=[SchemaObject(properties={}, additionalProperties=False)],
        ),
    }


def test_2_empty(simple_parse):
    parsed = simple_parse(
        {
            'allOf': [
                {
                    'type': 'object',
                    'properties': {},
                    'additionalProperties': False,
                },
                {
                    'type': 'object',
                    'properties': {},
                    'additionalProperties': False,
                },
            ],
        },
    )
    assert parsed.schemas == {
        'vfull#/definitions/type': AllOf(
            allOf=[
                SchemaObject(properties={}, additionalProperties=False),
                SchemaObject(properties={}, additionalProperties=False),
            ],
        ),
    }


def test_non_object(simple_parse):
    try:
        simple_parse({'allOf': [{'type': 'integer'}]})
        assert False
    except ParserError as exc:
        assert exc.infile_path == '/definitions/type/allOf/0'
        assert exc.msg == 'Non-object type in allOf: integer'
