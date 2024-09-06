from chaotic.front.parser import ParserError


def test_array_missing_items(simple_parse):
    try:
        simple_parse({'type': 'array'})
        assert False
    except ParserError as exc:
        assert exc.infile_path == '/definitions/type/items'
        assert exc.msg == '"items" is missing'


def test_int_array(simple_parse):
    simple_parse({'type': 'array', 'items': {'type': 'integer'}})


def test_full_array(simple_parse):
    simple_parse(
        {
            'type': 'array',
            'minItems': 1,
            'maxItems': 3,
            'items': {'type': 'integer'},
        },
    )


def test_int_array_array_array(simple_parse):
    simple_parse(
        {
            'type': 'array',
            'items': {
                'type': 'array',
                'items': {'type': 'array', 'items': {'type': 'integer'}},
            },
        },
    )
