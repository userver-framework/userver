# pylint: disable=too-many-lines
from collections.abc import Callable
import dataclasses
import itertools
from typing import Any

from chaotic import cpp_keywords
from chaotic.back.cpp import type_name
from chaotic.front import types

USERVER_COLONCOLON = 'userver::'


@dataclasses.dataclass
class CppType:
    raw_cpp_type: type_name.TypeName
    json_schema: types.Schema
    nullable: bool  # TODO: maybe move into  field?
    user_cpp_type: str | None

    _only_json_reason = ''

    KNOWN_X_PROPERTIES = []  # type: ignore

    def __hash__(self) -> int:
        return id(self)

    def is_isomorphic(self, other: 'CppType') -> bool:
        if self == other:
            return True

        left = self.json_schema.model_dump()
        right = other.json_schema.model_dump()
        return self._is_isomorphic_dicts(left, right)

    @staticmethod
    def _is_isomorphic_dicts(left: dict, right: dict) -> bool:
        left.pop('source_location_', None)
        right.pop('source_location_', None)
        left.pop('description', None)
        right.pop('description', None)
        if 'x_properties' in left:
            left['x_properties'].pop('description', None)
        if 'x_properties' in right:
            right['x_properties'].pop('description', None)

        if left.keys() != right.keys():
            return False

        for prop in left:
            p1 = left[prop]
            p2 = right[prop]

            if isinstance(p1, CppType) and isinstance(p2, CppType):
                if not p1.is_isomorphic(p2):
                    return False
            elif isinstance(p1, dict) and isinstance(p2, dict):
                if not CppType._is_isomorphic_dicts(p1, p2):
                    return False
            else:
                if p1 != p2:
                    return False

        return True

    def visit_children(self, cb: Callable[['CppType', 'CppType'], None]) -> None:
        for subtype in self.subtypes():
            cb(subtype, self)
            subtype.visit_children(cb)

    # Should return only direct subtypes, not recursively because
    # jinja's generate_*() is called recursively.
    def subtypes(self) -> list['CppType']:
        return []

    def declaration_includes(self) -> list[str]:
        raise NotImplementedError()

    def definition_includes(self) -> list[str]:
        raise NotImplementedError()

    def parser_type(self, ns: str, name: str) -> str:
        """
        C++ type for parser:

            field = json.As<Parser>():
                            ^^^^^^
        """
        raise NotImplementedError(self.raw_cpp_type)

    def get_py_type(self) -> str:
        return self.__class__.__name__

    def _cpp_name(self) -> str:
        """
        C++ type for declarations. May contain '@':

            namespace::Struct@Field
            ^^^^^^^^^^^^^^^^^^^^^^^
        """
        return self.raw_cpp_type.in_global_scope()

    def cpp_local_name(self) -> str:
        """
        C++ type in the parent struct:

            namespace::Struct::SubStruct
                               ^^^^^^^^^
        E.g. for type definition:

            class SubStruct { ... };
        """
        return self.raw_cpp_type.in_local_scope()

    def cpp_global_name(self) -> str:
        """
        C++ type in the global namespace:

            namespace::Struct::SubStruct
            ^^^^^^^^^^^^^^^^^^^^^^^^^^^^
        """
        return self._cpp_name()

    def cpp_user_name(self) -> str:
        """
        C++ type for field declaration:

            namespace::Struct::Field field;
            ^^^^^^^^^^^^^^^^^^^^^^^^
        """
        if not self.user_cpp_type:
            return self.cpp_global_name()
        cpp_type = self.user_cpp_type.replace('@', '::')
        if cpp_type.startswith(USERVER_COLONCOLON):
            cpp_type = 'USERVER_NAMESPACE::' + cpp_type[len(USERVER_COLONCOLON) :]
        return cpp_type

    def cpp_global_struct_field_name(self) -> str:
        """
        C++ name for global struct field declaration:

            namespace__Struct__Substruct
            ^^^^^^^^^^^^^^^^^^^^^^^^^^^^
        """
        return self.cpp_global_name().replace('::', '__')

    def cpp_comment(self) -> str:
        schema = self.json_schema
        description = ((schema.title or '') + '\n' + (schema.description or '')).strip()
        if description:
            return '// ' + description.replace('\n', '\n//')
        else:
            return ''

    @classmethod
    def get_includes_by_cpp_type(cls, cpp_type: str) -> list[str]:
        includes = []

        parts = cpp_type.split('<', 1)
        cpp_type = parts[0]
        if cpp_type == 'userver::utils::StrongTypedef' and parts[1][-1] == '>':
            cpp_type = parts[1].split(', ', 1)[0]
            includes.append('userver/utils/strong_typedef.hpp')
        elif cpp_type == 'utils::StrongTypedef' and parts[1][-1] == '>':
            # legacy uservices
            cpp_type = parts[1].split(', ', 1)[0]
            includes.append('userver/utils/strong_typedef.hpp')

        if cpp_type.startswith('::'):
            cpp_type = cpp_type[2:]

        includes.append('userver/chaotic/io/' + camel_to_snake_case(cpp_type.replace('::', '/')) + '.hpp')

        return includes

    def _primitive_parser_type(self) -> str:
        raw_cpp_type = f'USERVER_NAMESPACE::chaotic::Primitive<{self.raw_cpp_type.in_global_scope()}>'
        if self.user_cpp_type:
            user_cpp_type = self.cpp_user_name()
            return f'USERVER_NAMESPACE::chaotic::WithType<{raw_cpp_type}, {user_cpp_type}>'
        else:
            return raw_cpp_type

    def has_generated_user_cpp_type(self) -> bool:
        return False

    def need_dom_parser(self) -> bool:
        return False

    def may_generate_for_format(self, format_: str) -> bool:
        return format_ == 'USERVER_NAMESPACE::formats::json' or not self._only_json_reason

    def only_json_reason(self) -> str:
        return self._only_json_reason

    def need_serializer(self) -> bool:
        return False

    def need_operator_eq(self) -> bool:
        return False

    def need_using_type(self) -> bool:
        return False

    def need_operator_lshift(self) -> bool:
        return True


def camel_to_snake_case(string: str) -> str:
    parts = string.rsplit('/', 1)
    if len(parts) == 1:
        namespace = ''
        name = parts[0]
    else:
        namespace, name = parts

    result = ''
    for char in name:
        if char.isupper():
            if result:
                result += '_'
            result += char.lower()
        else:
            result += char
    if namespace:
        return namespace + '/' + result
    else:
        # e.g. unsigned
        return result


@dataclasses.dataclass
class CppPrimitiveValidator:
    min: int | float | None = None
    max: int | float | None = None
    exclusiveMin: int | float | None = None
    exclusiveMax: int | float | None = None
    minLength: int | None = None
    maxLength: int | None = None
    pattern: str | None = None

    namespace: str = ''
    prefix: str = ''

    # TODO: remove in TAXICOMMON-8626
    def fix_legacy_exclusive(self) -> None:
        if self.exclusiveMax is True:
            self.exclusiveMax = self.max
            self.max = None
        if self.exclusiveMax is False:
            self.exclusiveMax = None
        if self.exclusiveMin is True:
            self.exclusiveMin = self.min
            self.min = None
        if self.exclusiveMin is False:
            self.exclusiveMin = None

    def is_none(self) -> bool:
        return self == CppPrimitiveValidator()


# boolean, integer, number, string
@dataclasses.dataclass
class CppPrimitiveType(CppType):
    default: Any = None
    validators: CppPrimitiveValidator = dataclasses.field(
        default_factory=CppPrimitiveValidator,
    )

    KNOWN_X_PROPERTIES = [
        'x-usrv-cpp-type',
        'x-usrv-cpp-typedef-tag',
        'x-taxi-cpp-type',
        'x-taxi-cpp-typedef-tag',
    ]

    __hash__ = CppType.__hash__

    def declaration_includes(self) -> list[str]:
        includes = []
        if self.user_cpp_type:
            includes += self.get_includes_by_cpp_type(self.user_cpp_type)

        type_ = self.json_schema.type  # type: ignore
        if type_ == 'integer':
            includes.append('cstdint')
        elif type_ in ('number', 'boolean'):
            pass
        elif type_ in ('string', 'file'):
            includes.append('string')
        else:
            raise NotImplementedError(type_)
        return includes

    def definition_includes(self) -> list[str]:
        includes = ['userver/chaotic/primitive.hpp']
        if not self.validators.is_none():
            includes.append('userver/chaotic/validators.hpp')
        if self.validators.pattern:
            includes.append('userver/chaotic/validators_pattern.hpp')
        if self.user_cpp_type:
            includes.append('userver/chaotic/with_type.hpp')
        return includes

    def parser_type(self, ns: str, name: str) -> str:
        type_name_ns = self.validators.namespace
        type_name_decl = self.validators.prefix

        validators = ''
        if self.validators.min is not None:
            validators += f', USERVER_NAMESPACE::chaotic::Minimum<{type_name_ns}::k{type_name_decl}Minimum>'
        if self.validators.max is not None:
            validators += f', USERVER_NAMESPACE::chaotic::Maximum<{type_name_ns}::k{type_name_decl}Maximum>'

        if self.validators.exclusiveMin is not None:
            validators += (
                f', USERVER_NAMESPACE::chaotic::ExclusiveMinimum<{type_name_ns}::k{type_name_decl}ExclusiveMinimum>'
            )
        if self.validators.exclusiveMax is not None:
            validators += (
                f', USERVER_NAMESPACE::chaotic::ExclusiveMaximum<{type_name_ns}::k{type_name_decl}ExclusiveMaximum>'
            )

        if self.validators.minLength is not None:
            validators += f', USERVER_NAMESPACE::chaotic::MinLength<{self.validators.minLength}>'
        if self.validators.maxLength is not None:
            validators += f', USERVER_NAMESPACE::chaotic::MaxLength<{self.validators.maxLength}>'

        if self.validators.pattern is not None:
            validators += f', USERVER_NAMESPACE::chaotic::Pattern<{type_name_ns}::k{type_name_decl}Pattern>'

        parser_type = f'USERVER_NAMESPACE::chaotic::Primitive<{self.raw_cpp_type.in_global_scope()}{validators}>'
        if self.user_cpp_type:
            return f'USERVER_NAMESPACE::chaotic::WithType<{parser_type}, {self.cpp_user_name()}>'
        else:
            return parser_type

    def need_using_type(self) -> bool:
        return True

    def need_operator_lshift(self) -> bool:
        return False


@dataclasses.dataclass
class CppStringWithFormat(CppType):
    default: Any = None
    format_cpp_type: str = ''

    KNOWN_X_PROPERTIES = [
        'x-usrv-cpp-type',
        'x-usrv-cpp-typedef-tag',
        'x-taxi-cpp-type',
        'x-taxi-cpp-typedef-tag',
    ]

    __hash__ = CppType.__hash__

    def cpp_user_name(self) -> str:
        """
        C++ type for field declaration:

            namespace::Struct::Field field;
            ^^^^^^^^^^^^^^^^^^^^^^^^
        """
        if self.user_cpp_type:
            cpp_type = self.user_cpp_type.replace('@', '::')
        else:
            cpp_type = self.format_cpp_type.replace('@', '::')

        if cpp_type.startswith(USERVER_COLONCOLON):
            cpp_type = 'USERVER_NAMESPACE::' + cpp_type[len(USERVER_COLONCOLON) :]
        return cpp_type

    def declaration_includes(self) -> list[str]:
        includes = []
        if self.user_cpp_type:
            includes += self.get_includes_by_cpp_type(self.user_cpp_type)
        includes += self.get_includes_by_cpp_type(self.format_cpp_type)

        includes.append('string')
        return includes

    def definition_includes(self) -> list[str]:
        includes = ['userver/chaotic/primitive.hpp']
        includes.append('userver/chaotic/with_type.hpp')
        return includes

    def parser_type(self, ns: str, name: str) -> str:
        format_cpp_type = self.format_cpp_type
        if format_cpp_type.startswith(USERVER_COLONCOLON):
            format_cpp_type = 'USERVER_NAMESPACE::' + format_cpp_type[len(USERVER_COLONCOLON) :]
        parser_type = f'USERVER_NAMESPACE::chaotic::Primitive<{self.raw_cpp_type.in_global_scope()}>'
        parser_type = f'USERVER_NAMESPACE::chaotic::WithType<{parser_type}, {format_cpp_type}>'
        if self.user_cpp_type:
            parser_type = f'USERVER_NAMESPACE::chaotic::WithType<{parser_type}, {self.cpp_user_name()}>'
        return parser_type

    def need_using_type(self) -> bool:
        return True

    def need_operator_lshift(self) -> bool:
        return False


@dataclasses.dataclass
class CppRef(CppType):
    orig_cpp_type: CppType
    indirect: bool
    self_ref: bool
    cpp_name: str | None = None

    KNOWN_X_PROPERTIES = ['x-usrv-cpp-indirect', 'x-taxi-cpp-indirect']

    __hash__ = CppType.__hash__

    @property
    def default(self) -> str:
        return self.orig_cpp_type.default  # type: ignore

    def _cpp_name(self) -> str:
        if self.indirect:
            return f'USERVER_NAMESPACE::utils::Box<{self.orig_cpp_type._cpp_name()}>'
        else:
            return self.orig_cpp_type._cpp_name()

    def cpp_user_name(self) -> str:
        if self.indirect:
            return f'USERVER_NAMESPACE::utils::Box<{self.orig_cpp_type.cpp_user_name()}>'
        else:
            if not self.cpp_name or (self.orig_cpp_type.cpp_user_name() != self.orig_cpp_type.cpp_global_name()):
                # x-usrv-cpp-type
                return self.orig_cpp_type.cpp_user_name()
            else:
                return self.cpp_name

    def declaration_includes(self) -> list[str]:
        if self.indirect:
            return ['userver/utils/box.hpp']
        if self.self_ref:
            return []
        else:
            return self.orig_cpp_type.declaration_includes()

    def definition_includes(self) -> list[str]:
        if self.indirect:
            return ['userver/chaotic/ref.hpp']
        if self.self_ref:
            return []
        else:
            return self.orig_cpp_type.definition_includes()

    def parser_type(self, ns: str, name: str) -> str:
        if self.indirect:
            return f'USERVER_NAMESPACE::chaotic::Ref<{self.orig_cpp_type.parser_type(ns, name)}>'
        return self.orig_cpp_type.parser_type(ns, name)

    def need_using_type(self) -> bool:
        return True

    def need_operator_lshift(self) -> bool:
        return False


class EnumItemName(str):
    pass


@dataclasses.dataclass
class CppIntEnumItem:
    value: int
    raw_name: str
    cpp_name: str

    def declaration_includes(self) -> list[str]:
        return ['fmt/core.h']


@dataclasses.dataclass
class CppIntEnum(CppType):
    name: str
    enums: list[CppIntEnumItem]

    __hash__ = CppType.__hash__

    def declaration_includes(self) -> list[str]:
        return ['fmt/core.h']

    def definition_includes(self) -> list[str]:
        return [
            'cstdint',
            'userver/chaotic/exception.hpp',
            'userver/chaotic/primitive.hpp',
            'userver/utils/trivial_map.hpp',
        ]

    def parser_type(self, ns: str, name: str) -> str:
        return self._primitive_parser_type()

    def has_generated_user_cpp_type(self) -> bool:
        return True

    def need_dom_parser(self) -> bool:
        return True

    def need_serializer(self) -> bool:
        return True


@dataclasses.dataclass
class CppStringEnumItem:
    raw_name: str
    cpp_name: str

    def declaration_includes(self) -> list[str]:
        return ['fmt/core.h']


@dataclasses.dataclass
class CppStringEnum(CppType):
    name: str
    enums: list[CppStringEnumItem]
    default: EnumItemName | None

    __hash__ = CppType.__hash__

    def declaration_includes(self) -> list[str]:
        return ['string', 'fmt/core.h']

    def definition_includes(self) -> list[str]:
        return [
            'userver/chaotic/exception.hpp',
            'userver/chaotic/primitive.hpp',
            'userver/utils/trivial_map.hpp',
        ]

    def has_generated_user_cpp_type(self) -> bool:
        return True

    def parser_type(self, ns: str, name: str) -> str:
        return self._primitive_parser_type()

    def need_dom_parser(self) -> bool:
        return True

    def need_serializer(self) -> bool:
        return True


@dataclasses.dataclass
class CppStructPrimitiveField:
    raw_cpp_type: str
    user_cpp_type: str | None = None
    default: Any = None  # the type already checked at front stage


@dataclasses.dataclass
class CppStructField:
    name: str
    required: bool
    schema: CppType

    def _default(self) -> str | None:
        return getattr(self.schema, 'default', None)

    def _get_default(self) -> str:
        default = self._default()
        if default is None:
            return ''
        elif isinstance(default, EnumItemName):
            return f'{default}'
        elif isinstance(default, str):
            return f'"{default}"'
        elif isinstance(default, bool):
            if default:
                return 'true'
            else:
                return 'false'
        else:
            return default

    def get_default(self) -> str:
        default = self._get_default()
        if not default:
            return default

        if self.schema.user_cpp_type:
            if isinstance(default, str):
                default = f'std::string_view({default})'
            type_ = self.schema.user_cpp_type
            return f'Convert({default}, USERVER_NAMESPACE::chaotic::convert::To<{type_}>{{}})'
        return default

    def cpp_field_type(self) -> str:
        type_ = self.schema
        cpp_type = type_.cpp_user_name()

        if self.is_optional():
            return f'std::optional<{cpp_type}>'
        else:
            return cpp_type

    def is_optional(self) -> bool:
        optional = not self.required or self.schema.nullable
        return optional and self._default() is None

    def cpp_field_name(self) -> str:
        data = self.name
        if data[0].isnumeric():
            return 'x' + data
        elif cpp_keywords.is_cpp_keyword(data):
            return data + '_'
        else:
            return data

    def cpp_field_parse_type(self) -> str:
        type_ = self.schema.parser_type('TODO', self.name.title())
        if self.required or self._default() is not None:
            return type_
        else:
            return f'std::optional<{type_}>'


@dataclasses.dataclass
class CppStruct(CppType):
    fields: dict[str, CppStructField]
    # 'None' means 'do not generate extra member'
    extra_type: CppType | bool | None = False
    autodiscover_default_dict: bool = False
    strict_parsing: bool = True

    KNOWN_X_PROPERTIES = [
        'x-usrv-cpp-type',
        'x-usrv-cpp-extra-type',
        'x-usrv-cpp-extra-member',
        'x-taxi-cpp-type',
        'x-taxi-cpp-extra-type',
        'x-taxi-cpp-extra-member',
    ]

    __hash__ = CppType.__hash__

    def __post_init__(self) -> None:
        if self._is_default_dict():
            self.extra_type = self.fields['__default__'].schema
            self.fields['__default__'].required = False

    def _is_default_dict_candidate(self) -> bool:
        if len(self.fields) != 1:
            return False
        default = self.fields.get('__default__')
        if default and isinstance(self.extra_type, CppType) and default.schema.is_isomorphic(self.extra_type):
            return True
        return False

    def _is_default_dict(self) -> bool:
        return self.autodiscover_default_dict and self._is_default_dict_candidate()

    def cpp_user_name(self) -> str:
        if self._is_default_dict():
            assert isinstance(self.extra_type, CppType)
            return f'USERVER_NAMESPACE::utils::DefaultDict<{self.extra_type.cpp_user_name()}>'
        return super().cpp_user_name()

    def parser_type(self, ns: str, name: str) -> str:
        parser_type = self._primitive_parser_type()
        if self._is_default_dict():
            assert isinstance(self.extra_type, CppType)
            dict_type = f'USERVER_NAMESPACE::utils::DefaultDict<{self.extra_type.cpp_user_name()}>'
            return f'USERVER_NAMESPACE::chaotic::WithType<{parser_type}, {dict_type}>'
        return parser_type

    def subtypes(self) -> list[CppType]:
        types = [field.schema for field in self.fields.values()]
        if isinstance(self.extra_type, CppType) and not self._is_default_dict():
            types.append(self.extra_type)
        return types

    def extra_container(self) -> str:
        kwargs = self.json_schema.x_properties
        return kwargs.get(
            'x-usrv-cpp-extra-type',
            kwargs.get('x-taxi-cpp-extra-type', 'std::unordered_map'),
        )

    def declaration_includes(self) -> list[str]:
        includes = ['optional']
        if self.user_cpp_type:
            includes += self.get_includes_by_cpp_type(self.user_cpp_type)
        for field in self.fields.values():
            includes.extend(field.schema.declaration_includes())

        if self.extra_type:
            includes.append('string')
            if isinstance(self.extra_type, CppType):
                extra_container = self.extra_container()
                includes += self.get_includes_by_cpp_type(extra_container)
                includes.extend(self.extra_type.declaration_includes())
            else:
                includes.append('userver/formats/json/value.hpp')

        if self._is_default_dict():
            includes.append('userver/utils/default_dict.hpp')
        return includes

    def definition_includes(self) -> list[str]:
        includes = [
            'userver/formats/parse/common_containers.hpp',
            'userver/formats/serialize/common_containers.hpp',
            'userver/chaotic/primitive.hpp',
            'userver/chaotic/with_type.hpp',
        ]
        if self.extra_type or self.strict_parsing:
            # for ExtractAdditionalProperties/ValidateNoAdditionalProperties
            includes.append('userver/chaotic/object.hpp')

        if self.extra_type:
            # for kPropertiesNames
            includes.append('userver/utils/trivial_map.hpp')
        for field in self.fields.values():
            includes.extend(field.schema.definition_includes())
        if isinstance(self.extra_type, CppType):
            includes.extend(self.extra_type.definition_includes())
        if self._is_default_dict():
            includes += self.get_includes_by_cpp_type(
                'userver::utils::DefaultDict<>',
            )
        return includes

    def need_dom_parser(self) -> bool:
        return True

    def need_serializer(self) -> bool:
        return True

    def need_operator_eq(self) -> bool:
        return True


@dataclasses.dataclass
class CppArrayValidator:
    minItems: int | None = None
    maxItems: int | None = None

    def is_none(self) -> bool:
        return self == CppArrayValidator()


@dataclasses.dataclass
class CppArray(CppType):
    items: CppType
    container: str
    validators: CppArrayValidator

    KNOWN_X_PROPERTIES = [
        'x-usrv-cpp-type',
        'x-usrv-cpp-container',
        'x-taxi-cpp-type',
        'x-taxi-cpp-container',
    ]

    __hash__ = CppType.__hash__

    def _cpp_name(self) -> str:
        return f'{self.container}<{self.items.cpp_user_name()}>'

    def subtypes(self) -> list[CppType]:
        return [self.items]

    def parser_type(self, ns: str, name: str) -> str:
        validators = ''
        if self.validators.minItems is not None:
            validators += f', USERVER_NAMESPACE::chaotic::MinItems<{self.validators.minItems}>'
        if self.validators.maxItems is not None:
            validators += f', USERVER_NAMESPACE::chaotic::MaxItems<{self.validators.maxItems}>'

        parser_type = (
            'USERVER_NAMESPACE::chaotic::Array'
            f'<{self.items.parser_type(ns, name)}, '
            f'{self.container}<{self.items.cpp_user_name()}>{validators}>'
        )
        user_cpp_type = self.user_cpp_type
        if user_cpp_type:
            parser_type = f'USERVER_NAMESPACE::chaotic::WithType<{parser_type}, {user_cpp_type}>'
        return parser_type

    def declaration_includes(self) -> list[str]:
        includes = self.get_includes_by_cpp_type(self.container) + self.items.declaration_includes()
        if self.user_cpp_type:
            includes += self.get_includes_by_cpp_type(self.user_cpp_type)
        return includes

    def definition_includes(self) -> list[str]:
        return [
            'userver/chaotic/array.hpp',
            'userver/chaotic/with_type.hpp',
        ] + self.items.definition_includes()

    def need_using_type(self) -> bool:
        return True

    def need_operator_lshift(self) -> bool:
        return False


def flatten(data: list) -> list:
    return list(itertools.chain.from_iterable(data))


@dataclasses.dataclass
class CppStructAllOf(CppType):
    parents: list[CppType]

    KNOWN_X_PROPERTIES = ['x-usrv-cpp-type', 'x-taxi-cpp-type']

    __hash__ = CppType.__hash__

    def declaration_includes(self) -> list[str]:
        includes = []
        if self.user_cpp_type:
            includes += self.get_includes_by_cpp_type(self.user_cpp_type)
        includes += flatten([item.declaration_includes() for item in self.parents])
        return includes

    def definition_includes(self) -> list[str]:
        return [
            'userver/formats/common/merge.hpp',
            'userver/chaotic/primitive.hpp',
        ] + flatten([item.definition_includes() for item in self.parents])

    def parser_type(self, ns: str, name: str) -> str:
        return self._primitive_parser_type()

    def subtypes(self) -> list[CppType]:
        return self.parents

    def need_dom_parser(self) -> bool:
        return True

    def need_serializer(self) -> bool:
        return True

    def need_operator_eq(self) -> bool:
        return True


@dataclasses.dataclass
class CppVariant(CppType):
    variants: list[CppType]

    KNOWN_X_PROPERTIES = ['x-usrv-cpp-type', 'x-taxi-cpp-type']

    __hash__ = CppType.__hash__

    def declaration_includes(self) -> list[str]:
        includes = []
        if self.user_cpp_type:
            includes += self.get_includes_by_cpp_type(self.user_cpp_type)
        return (
            includes
            + [
                'variant',
                'userver/formats/parse/variant.hpp',
                'userver/formats/json/serialize_variant.hpp',
            ]
            + flatten([item.declaration_includes() for item in self.variants])
        )

    def definition_includes(self) -> list[str]:
        return [
            'userver/chaotic/primitive.hpp',
            'userver/chaotic/variant.hpp',
        ] + flatten([item.definition_includes() for item in self.variants])

    def parser_type(self, ns: str, name: str) -> str:
        parsers = []
        for variant in self.variants:
            parsers.append(variant.parser_type(ns, name))
        parser_type = f'USERVER_NAMESPACE::chaotic::Variant<{",".join(parsers)}>'
        if self.user_cpp_type:
            return f'USERVER_NAMESPACE::chaotic::WithType<{parser_type}, {self.cpp_user_name()}>'
        else:
            return parser_type

    def subtypes(self) -> list[CppType]:
        return self.variants

    def need_operator_lshift(self) -> bool:
        return False


@dataclasses.dataclass
class CppVariantWithDiscriminator(CppType):
    property_name: str
    variants: dict[str, CppType]
    mapping_type: types.MappingType = types.MappingType.STR

    KNOWN_X_PROPERTIES = ['x-usrv-cpp-type', 'x-taxi-cpp-type']

    __hash__ = CppType.__hash__

    def declaration_includes(self) -> list[str]:
        includes = ['variant', 'userver/chaotic/oneof_with_discriminator.hpp']

        if self.user_cpp_type:
            includes += self.get_includes_by_cpp_type(self.user_cpp_type)
        return includes + flatten([item.declaration_includes() for item in self.variants.values()])

    def definition_includes(self) -> list[str]:
        return ['userver/formats/json/serialize_variant.hpp'] + flatten([
            item.definition_includes() for item in self.variants.values()
        ])

    def parser_type(self, ns: str, name: str) -> str:
        variants_list = []
        for variant in self.variants.values():
            variants_list.append(variant.parser_type(ns, name))
        variants = ', '.join(variants_list)

        settings_name = (
            self.raw_cpp_type.parent().joinns(
                'k' + self.cpp_local_name() + '_Settings',
            )
        ).in_global_scope()

        parser_type = f'USERVER_NAMESPACE::chaotic::OneOfWithDiscriminator<&{settings_name}, {variants}>'
        if self.user_cpp_type:
            return f'USERVER_NAMESPACE::chaotic::WithType<{parser_type}, {self.cpp_user_name()}>'
        else:
            return parser_type

    def need_operator_lshift(self) -> bool:
        return False

    def is_str_discriminator(self) -> bool:
        return self.mapping_type == types.MappingType.STR

    def is_int_discriminator(self) -> bool:
        return self.mapping_type == types.MappingType.INT
