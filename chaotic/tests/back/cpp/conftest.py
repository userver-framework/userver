from collections import OrderedDict
from typing import Dict

import pytest

from chaotic.back.cpp.translator import Generator
from chaotic.back.cpp.translator import GeneratorConfig
from chaotic.back.cpp.types import CppType


@pytest.fixture
def simple_gen(simple_parse, clean):
    def func(input_: dict):
        schemas = simple_parse(input_)
        gen = Generator(
            config=GeneratorConfig(namespaces={'vfull': ''}, include_dirs=None),
        )
        types = gen.generate_types(schemas)
        return clean(types)

    return func


@pytest.fixture(name='clean')
def _clean():
    def func(ordered_dict: OrderedDict) -> Dict[str, CppType]:
        res = {}
        for key, val in ordered_dict.items():
            res[key] = val.without_json_schema()
        return res

    return func
