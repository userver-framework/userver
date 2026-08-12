from collections import OrderedDict

import pytest

from chaotic import main
from chaotic.back.cpp import translator
from chaotic.back.cpp import types as cpp_types
from chaotic.front import types as front_types


@pytest.fixture
def cpp_name_func():
    return main.generate_cpp_name_func(
        [main.NameMapItem('/definitions/([^/]*)/={0}'), main.NameMapItem('/([^/]*)/={0}')],
        '',
    )


@pytest.fixture
def simple_gen(simple_parse, clean, cpp_name_func):
    def func(input_: dict):
        schemas = simple_parse(input_, clear=False)
        gen = translator.Generator(
            config=translator.GeneratorConfig(
                namespaces={'vfull': ''}, include_dirs=None, infile_to_name_func=cpp_name_func
            ),
        )
        types = gen.generate_types(schemas)
        return clean(types)

    return func


@pytest.fixture(name='clean')
def _clean():
    def func(ordered_dict: OrderedDict) -> dict[str, cpp_types.CppType]:
        res = {}

        def visit(child: cpp_types.CppType, parent: cpp_types.CppType):
            child.json_schema = front_types.Schema()

        for key, val in ordered_dict.items():
            visit(val, None)
            val.visit_children(visit)
            res[key] = val
        return res

    return func
