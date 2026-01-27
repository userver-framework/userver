"""`TypeReference` constants for common built-in C++ types."""

from collections.abc import Mapping

from proto_structs.models import type_ref

#: `std::optional`
OPTIONAL_TEMPLATE = type_ref.BuiltinType(full_cpp_name='std::optional')
#: `std::vector`
VECTOR_TEMPLATE = type_ref.BuiltinType(full_cpp_name='std::vector')
#: `USERVER_NAMESPACE::proto_structs::HashMap`
HASH_MAP_TEMPLATE = type_ref.UserverLibraryType(full_cpp_name_wo_userver='proto_structs::HashMap')

#: `USERVER_NAMESPACE::utils::Box`
BOX_TEMPLATE = type_ref.UserverLibraryType(full_cpp_name_wo_userver='utils::Box')
#: `USERVER_NAMESPACE::proto_structs::UnbreakableDependencyCycle`
UNBREAKABLE_DEPENDENCY_CYCLE = type_ref.UserverLibraryType(
    full_cpp_name_wo_userver='proto_structs::UnbreakableDependencyCycle',
)

#: `USERVER_NAMESPACE::proto_structs::Oneof`
ONEOF_BASE_CLASS = type_ref.UserverLibraryType(full_cpp_name_wo_userver='proto_structs::Oneof')

#: All Protobuf primitive types, mapped from their Protobuf names to C++ types.
PRIMITIVE_TYPES: Mapping[str, type_ref.TypeReference] = {
    'bool': type_ref.KeywordType(full_cpp_name='bool'),
    'float': type_ref.KeywordType(full_cpp_name='float'),
    'double': type_ref.KeywordType(full_cpp_name='double'),
    'string': type_ref.BuiltinType(full_cpp_name='std::string'),
    'bytes': type_ref.BuiltinType(full_cpp_name='std::string'),
    'int32': type_ref.BuiltinType(full_cpp_name='std::int32_t'),
    'int64': type_ref.BuiltinType(full_cpp_name='std::int64_t'),
    'uint32': type_ref.BuiltinType(full_cpp_name='std::uint32_t'),
    'uint64': type_ref.BuiltinType(full_cpp_name='std::uint64_t'),
    'sint32': type_ref.BuiltinType(full_cpp_name='std::int32_t'),
    'sint64': type_ref.BuiltinType(full_cpp_name='std::int64_t'),
    'fixed32': type_ref.BuiltinType(full_cpp_name='std::uint32_t'),
    'fixed64': type_ref.BuiltinType(full_cpp_name='std::uint64_t'),
    'sfixed32': type_ref.BuiltinType(full_cpp_name='std::int32_t'),
    'sfixed64': type_ref.BuiltinType(full_cpp_name='std::int64_t'),
}

#: All Protobuf well-known types, mapped from their Protobuf names to C++ types.
WELL_KNOWN_TYPES: Mapping[str, type_ref.TypeReference] = {
    'google.protobuf.Timestamp': type_ref.UserverLibraryType(full_cpp_name_wo_userver='proto_structs::Timestamp'),
    'google.protobuf.Duration': type_ref.UserverLibraryType(full_cpp_name_wo_userver='proto_structs::Duration'),
    'google.type.Date': type_ref.UserverLibraryType(full_cpp_name_wo_userver='proto_structs::Date'),
    'google.type.TimeOfDay': type_ref.UserverLibraryType(full_cpp_name_wo_userver='proto_structs::TimeOfDay'),
    'google.protobuf.Any': type_ref.UserverLibraryType(full_cpp_name_wo_userver='proto_structs::Any'),
}


def make_optional(value: type_ref.TypeReference) -> type_ref.TypeReference:
    """Makes `std::optional<T>` type."""
    return type_ref.TemplateType(template=OPTIONAL_TEMPLATE, template_args=(value,))


def make_vector(value: type_ref.TypeReference) -> type_ref.TypeReference:
    """Makes `std::vector<T>` type."""
    return type_ref.TemplateType(
        template=VECTOR_TEMPLATE,
        template_args=(value,),
        works_with_forward_references=True,
    )


def make_hash_map(key: type_ref.TypeReference, value: type_ref.TypeReference) -> type_ref.TypeReference:
    """Makes `USERVER_NAMESPACE::proto_structs::HashMap<K, V>` type."""
    return type_ref.TemplateType(
        template=HASH_MAP_TEMPLATE,
        template_args=(
            key,
            value,
        ),
    )


def make_box(value: type_ref.TypeReference) -> type_ref.TypeReference:
    """Makes `USERVER_NAMESPACE::utils::Box<T>` type."""
    return type_ref.TemplateType(template=BOX_TEMPLATE, template_args=(value,), works_with_forward_references=True)
