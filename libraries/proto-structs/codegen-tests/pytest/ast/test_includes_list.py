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
        ),
        pytest.param(
            'maps/basic.proto',
            [
                'userver/proto-structs/io/userver/proto_structs/hash_map.hpp',
                'userver/proto-structs/io/userver/proto_structs/hash_map_conv.hpp',
            ],
        ),
        pytest.param(
            'box/autobox/unbreakable_cycle.proto',
            [
                'userver/proto-structs/io/userver/utils/box.hpp',
                'userver/proto-structs/io/userver/utils/box_conv.hpp',
            ],
        ),
    ],
)
def test_includes_are_correct(proto_path, expected_includes) -> None:
    proto_source = utils.load_proto_source_file(proto_path)
    file_ast = parser.Parser().parse(proto_source)
    includes = includes_collection.collect(file_ast=file_ast, plugin_options=None)
    assert includes == expected_includes
