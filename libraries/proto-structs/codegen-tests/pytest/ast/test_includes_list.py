from proto_schema_parser import ast
from proto_schema_parser import parser
from proto_structs.ast import includes_collection
import pytest

import utils


@pytest.mark.parametrize(
    ('proto_path', 'expected_includes'),
    [
        pytest.param(
            'enums/names.proto',
            [],
            id='enums',
        ),
        pytest.param(
            'maps/basic.proto',
            [
                'userver/proto-structs/io/userver/proto_structs/hash_map.hpp',
                'userver/proto-structs/io/userver/proto_structs/hash_map_conv.hpp',
            ],
            id='maps',
        ),
        pytest.param(
            'box/autobox/unbreakable_cycle.proto',
            [],
            id='unbreakable_cycle',
        ),
    ],
)
def test_includes_are_correct(proto_path, expected_includes) -> None:
    proto_source = utils.load_proto_source_file(proto_path)
    file_ast = parser.Parser().parse(proto_source)
    includes = includes_collection.collect(file_ast=file_ast, plugin_options=None)
    assert includes == expected_includes


@pytest.mark.parametrize(
    ('file_ast', 'expected_includes'),
    [
        pytest.param(
            ast.File(
                syntax='proto3',
                file_elements=[
                    ast.Package(name='google.protobuf'),
                    ast.Import(name='google/protobuf/any.proto'),
                    ast.Message(
                        name='Option',
                        elements=[
                            ast.Field(
                                name='name',
                                number='1',
                                type='string',
                            ),
                            ast.Field(
                                name='value',
                                number=2,
                                type='Any',  # <===== Type reference without package.
                            ),
                        ],
                    ),
                ],
            ),
            [
                'userver/proto-structs/io/userver/proto_structs/any.hpp',
                'userver/proto-structs/io/userver/proto_structs/any_conv.hpp',
            ],
            id='type_to_any',
        ),
    ],
)
def test_deps_within_well_knowns(file_ast, expected_includes) -> None:
    includes = includes_collection.collect(file_ast=file_ast, plugin_options=None)
    assert includes == expected_includes
