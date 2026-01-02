from chaotic_openapi.front import parser as front_parser
import pytest


@pytest.fixture
def simple_parser():
    def clear_source_location(schema):
        schema.source_location_ = None

    def _simple_parser(schema):
        parser = front_parser.Parser('test')
        parser.parse_schema(schema, '<inline>', '<inline>')
        parser.service().visit_all_schemas(clear_source_location)
        return parser.service()

    return _simple_parser
