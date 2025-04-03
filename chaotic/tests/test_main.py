import chaotic.main as main


def test_name_map_item():
    item = main.NameMapItem('x=y')
    assert item.match('x') == 'y'
    assert item.match('y') is None


def test_name_map_item_pattern():
    item = main.NameMapItem('/schemas/([^/]*)/={0}')
    assert item.match('x') is None
    assert item.match('/schemas') is None
    assert item.match('/schemas/') is None
    assert item.match('/schemas/X/') == 'X'


def test_extract_schemas_to_scan():
    items = [
        main.NameMapItem('/schemas/([^/]*)/={0}'),
        main.NameMapItem('/definitions/([^/]*)/={0}'),
    ]
    data = {
        'schemas': {'X': {'type': 'boolean'}, 'Y': {'type': 'integer'}},
        'definitions': {'Z': {'type': 'number'}},
    }

    assert main.extract_schemas_to_scan(data, items) == {
        '/definitions/Z/': {'type': 'number'},
        '/schemas/X/': {'type': 'boolean'},
        '/schemas/Y/': {'type': 'integer'},
    }


def test_extract_schemas_to_scan_configs():
    items = [
        main.NameMapItem('/schema/definitions([^/]*)/={0}'),
        main.NameMapItem('/schema/=Type'),
    ]
    data = {
        'schema': {
            'type': 'object',
            'properties': {'x': {'$ref': '#/definitions/Z'}},
            'definitions': {'Z': {'type': 'number'}},
        },
    }

    assert main.extract_schemas_to_scan(data, items) == {
        '/schema/definitions/': {'Z': {'type': 'number'}},
        '/schema/': {
            'type': 'object',
            'properties': {'x': {'$ref': '#/definitions/Z'}},
        },
    }
