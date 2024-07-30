import pytest

from chaotic.front import parser


@pytest.fixture
def simple_parse():
    def func(input_: dict):
        config = parser.ParserConfig(erase_prefix='')
        schema_parser = parser.SchemaParser(
            config=config, full_filepath='full', full_vfilepath='vfull',
        )
        schema_parser.parse_schema('/definitions/type', input_)
        return schema_parser.parsed_schemas()

    return func
