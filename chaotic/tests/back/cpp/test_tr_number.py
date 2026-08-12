import pytest

from chaotic.back.cpp import types as cpp_types
from chaotic.front import parser as front_parser


def test_number(simple_gen, cpp_primitive_type):
    types = simple_gen({'type': 'number'})
    assert types == {
        '::type': cpp_primitive_type(
            validators=cpp_types.CppPrimitiveValidator(prefix='type'),
            raw_cpp_type_str='double',
        ),
    }


@pytest.mark.parametrize('format_', ['float', 'double'])
def test_format(simple_gen, cpp_primitive_type, format_):
    types = simple_gen({'type': 'number', 'format': format_})
    assert types == {
        '::type': cpp_primitive_type(
            validators=cpp_types.CppPrimitiveValidator(prefix='type'),
            raw_cpp_type_str='double',
        ),
    }


def test_number_minmax(simple_gen, cpp_primitive_type):
    types = simple_gen({'type': 'number', 'minimum': 1, 'maximum': 2.2})
    assert types == {
        '::type': cpp_primitive_type(
            validators=cpp_types.CppPrimitiveValidator(
                min=1,
                max=2.2,
                prefix='type',
            ),
            raw_cpp_type_str='double',
        ),
    }


def test_number_minmax_exclusive(simple_gen, cpp_primitive_type):
    types = simple_gen({
        'type': 'number',
        'exclusiveMinimum': 1,
        'exclusiveMaximum': 2.2,
    })
    assert types == {
        '::type': cpp_primitive_type(
            validators=cpp_types.CppPrimitiveValidator(
                exclusiveMin=1,
                exclusiveMax=2.2,
                prefix='type',
            ),
            raw_cpp_type_str='double',
        ),
    }


def test_number_extra_enum(simple_gen):
    with pytest.raises(front_parser.ParserError) as exc:
        simple_gen({'type': 'number', 'enum': [1.0]})
    assert exc.value.infile_path == '/definitions/type'
    assert 'Unknown field: "enum"' in exc.value.msg
