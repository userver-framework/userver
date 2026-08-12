import pytest

from chaotic.back.cpp import translator
from chaotic.back.cpp import type_name
from chaotic.back.cpp import types as cpp_types
from chaotic.front import parser
from chaotic.front import ref_resolver
from chaotic.front import types


def clear_source_location(child: types.Schema, _) -> None:
    child.source_location_ = None


def parse(path, input_, external_schemas, external_types, cpp_name_func):
    config = parser.ParserConfig(erase_prefix='')
    schema_parser = parser.SchemaParser(
        config=config,
        full_filepath='full',
        full_vfilepath='vfull',
    )
    schema_parser.parse_schema(path, input_)
    schemas = schema_parser.parsed_schemas()
    for schema in schemas.schemas.values():
        schema.visit_children(clear_source_location)
        clear_source_location(schema, None)

    rr = ref_resolver.RefResolver()
    resolved_schemas = rr.sort_schemas(
        schemas,
        external_schemas=external_schemas,
    )
    gen = translator.Generator(
        config=translator.GeneratorConfig(
            namespaces={'vfull': ''}, include_dirs=None, infile_to_name_func=cpp_name_func
        ),
    )
    types = gen.generate_types(
        resolved_schemas,
        external_schemas=external_types,
    )

    return resolved_schemas, types


def test_import(cpp_name_func, cpp_primitive_type, clean):
    ext_schemas, ext_types = parse('/type1', {'type': 'string'}, types.ResolvedSchemas(schemas={}), {}, cpp_name_func)
    assert ext_schemas.schemas == {'vfull#/type1': types.String(type='string')}
    assert ext_types == {
        '::type1': cpp_primitive_type(
            validators=cpp_types.CppPrimitiveValidator(prefix='type1'),
            raw_cpp_type_str='std::string',
            json_schema=types.String(type='string'),
        ),
    }

    new_schemas, new_types = parse('/type2', {'$ref': 'vfull#/type1'}, ext_schemas, ext_types, cpp_name_func)
    assert new_schemas.schemas == {
        'vfull#/type2': types.Ref(
            ref='vfull#/type1',
            indirect=False,
            self_ref=False,
            schema_=ext_schemas.schemas['vfull#/type1'],
        ),
    }
    clean(new_types)
    assert new_types == {
        '::type2': cpp_types.CppRef(
            orig_cpp_type=ext_types['::type1'],
            indirect=False,
            self_ref=False,
            raw_cpp_type=type_name.TypeName('::type2'),
            nullable=False,
            user_cpp_type=None,
            json_schema=types.Schema(),
        ),
    }


def test_duplicate_name(cpp_name_func):
    config = parser.ParserConfig(erase_prefix='')

    schema_parser = parser.SchemaParser(
        config=config,
        full_filepath='full',
        full_vfilepath='vfull',
    )
    schema_parser.parse_schema('/type1', {'type': 'string'})
    schemas1 = schema_parser.parsed_schemas()

    schema_parser = parser.SchemaParser(
        config=config,
        full_filepath='full2',
        full_vfilepath='vfull2',
    )
    schema_parser.parse_schema('/type1', {'type': 'integer'})
    schemas2 = schema_parser.parsed_schemas()

    schemas = types.ParsedSchemas.merge([schemas1, schemas2])
    rr = ref_resolver.RefResolver()
    resolved_schemas = rr.sort_schemas(
        schemas,
    )

    gen = translator.Generator(
        config=translator.GeneratorConfig(
            namespaces={'vfull': '', 'vfull2': ''},
            include_dirs=None,
            infile_to_name_func=cpp_name_func,
        ),
    )

    with pytest.raises(
        BaseException,
        match='Duplicate type name: ::type1, generated from vfull2#/type1 and vfull#/type1',
    ):
        gen.generate_types(
            resolved_schemas,
        )
