from chaotic_openapi.back.cpp_client import translator
from chaotic_openapi.front import parser as front_parser
import pytest


@pytest.fixture
def translate_single_schema():
    def func(schema):
        parser = front_parser.Parser('test')
        parser.parse_schema(schema, '<inline>')
        service = parser.service()

        tr = translator.Translator(
            service,
            dynamic_config='',
            cpp_namespace='test_namespace',
            include_dirs=[],
        )
        return tr.spec()

    return func
