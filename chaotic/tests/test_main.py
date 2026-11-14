from chaotic import main


def test_name_map_item():
    item = main.NameMapItem('x=y')
    assert item.match('x', stem='stem') == 'y'
    assert item.match('y', stem='stem') is None


def test_name_map_item_pattern():
    item = main.NameMapItem('/schemas/([^/]*)/={0}')
    assert item.match('x', stem='stem') is None
    assert item.match('/schemas', stem='stem') is None
    assert item.match('/schemas/', stem='stem') is None
    assert item.match('/schemas/X/', stem='stem') == 'X'


def test_cpp_name_func_stem_is_cpp_identifier():
    func = main.generate_cpp_name_func([main.NameMapItem('/schemas/([^/]*)/=ns::{stem}::{0}')], '')
    assert func('/schemas/X', 'types') == 'ns::types::X'
    assert func('/schemas/X', 'station.internal') == 'ns::station_internal::X'
    assert func('/schemas/X', 'station-api.v1') == 'ns::station_api_v1::X'
    assert func('/schemas/X', '2024') == 'ns::x2024::X'


def test_file_map_keeps_stem():
    item = main.NameMapItem('(.*)/([^/]*)\\.yaml={0}/{stem}')
    assert main.vfilepath_from_filepath('schemas/station.internal.yaml', [item]) == 'schemas/station.internal'


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


def test_extract_schemas_to_scan_no_duplicate_deletion():
    items = [main.NameMapItem('/.*=SomeType')]
    data = {
        'schema': {
            'field': {'type': 'integer'},
        },
    }
    result = main.extract_schemas_to_scan(data, items)
    assert result['/schema/field/'] == {'type': 'integer'}
