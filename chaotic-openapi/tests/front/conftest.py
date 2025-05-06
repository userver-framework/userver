from chaotic_openapi.front import parser as front_parser
import pytest


@pytest.fixture
def simple_parser():
    def _simple_parser(schema):
        parser = front_parser.Parser('test')
        parser.parse_schema(schema, '<inline>')
        return parser.service()

    return _simple_parser
