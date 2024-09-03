# pylint: disable=too-many-lines
import dataclasses
import itertools
from typing import Any
from typing import Dict
from typing import List
from typing import Optional
from typing import Union

from chaotic.back.cpp import keywords as cpp_keywords
from chaotic.back.cpp import type_name
from chaotic.front import types

USERVER_COLONCOLON = 'userver::'


@dataclasses.dataclass
class CppType:
    raw_cpp_type: type_name.TypeName
    json_schema: Optional[types.Schema]
    nullable: bool  # TODO: maybe move into  field?
    user_cpp_type: Optional[str]

    _only_json_reason = ''

    KNOWN_X_PROPERTIES = []  # type: ignore

    def __hash__(self) -> int:
        return id(self)

    def is_isomorphic(self, other: 'CppType') -> bool:
        left = dataclasses.asdict(self.json_schema)
        left.pop('description', None)
        left['x_properties'].pop('description', None)

        right = dataclasses.asdict(other.json_schema)
        right.pop('description', None)
        right['x_properties'].pop('description', None)

        return left == right

    def without_json_schema(self) -> 'CppType':
        return dataclasses.replace(self, json_schema=None)

    # Should return only direct subtypes, not recursively because
    # jinja's generate_*() is called recursively.
    def subtypes(self) -> List['CppType']:
        return []

    def declaration_includes(self) -> List[str]:
        raise NotImplementedError()

    def definition_includes(self) -> List[str]:
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
            cpp_type = (
                'USERVER_NAMESPACE::' + cpp_type[len(USERVER_COLONCOLON) :]
            )
        return cpp_type

    def cpp_global_struct_field_name(self) -> str:
        """
        C++ name for global struct field declaration:

            namespace__Struct__Substruct
            ^^^^^^^^^^^^^^^^^^^^^^^^^^^^
        """
        return self.cpp_global_name().replace('::', '__')

    def cpp_comment(self) -> str:
        assert self.json_schema

        kwargs = self.json_schema.x_properties
        description = kwargs.get('description')
        if description:
            return '// ' + description.replace('\n', '\n//')
        else:
            return ''

    @classmethod
    def cpp_type_to_user_include_path(cls, cpp_type: str) -> str:
        parts = cpp_type.split('<', 1)
        cpp_type = parts[0]
        if cpp_type == 'userver::utils::StrongTypedef' and parts[1][-1] == '>':
            cpp_type = parts[1].split(', ', 1)[0]

        if cpp_type.startswith('::'):
            cpp_type = cpp_type[2:]
        return (
            'userver/chaotic/io/'
            + camel_to_snake_case(cpp_type.replace('::', '/'))
            + '.hpp'
        )

    @classmethod
    def get_include_by_cpp_type(cls, cpp_type: str) -> List[str]:
        return [cls.cpp_type_to_user_include_path(cpp_type)]

    def _primitive_parser_type(self) -> str:
        raw_cpp_type = (
            'USERVER_NAMESPACE::chaotic::Primitive'
            f'<{self.raw_cpp_type.in_global_scope()}>'
        )
        if self.user_cpp_type:
            user_cpp_type = self.cpp_user_name()
            return (
                f'USERVER_NAMESPACE::chaotic::WithType<{raw_cpp_type}, '
                f'{user_cpp_type}>'
            )
        else:
            return raw_cpp_type

    def has_generated_user_cpp_type(self) -> bool:
        return False

    def need_dom_parser(self) -> bool:
        return False

    def may_generate_for_format(self, format_: str) -> bool:
        return (
            format_ == 'USERVER_NAMESPACE::formats::json'
            or not self._only_json_reason
        )

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
    min: Union[Optional[int], float] = None
    max: Union[Optional[int], float] = None
    exclusiveMin: Union[Optional[int], float] = None
    exclusiveMax: Union[Optional[int], float] = None
    minLength: Optional[int] = None
    maxLength: Optional[int] = None
    pattern: Optional[str] = None

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
    default: Optional[Any] = None
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

    def declaration_includes(self) -> List[str]:
        includes = []
        if self.user_cpp_type:
            includes += self.get_include_by_cpp_type(self.user_cpp_type)

        assert self.json_schema
        type_ = self.json_schema.type  # type: ignore
        if type_ == 'integer':
            includes.append('cstdint')
        elif type_ in ('number', 'boolean'):
            pass
        elif type_ == 'string':
            includes.append('string')
        else:
            raise NotImplementedError(type_)
        return includes

    def definition_includes(self) -> List[str]:
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
            validators += (
                ', USERVER_NAMESPACE::chaotic::Minimum'
                f'<{type_name_ns}::k{type_name_decl}Minimum>'
            )
        if self.validators.max is not None:
            validators += (
                ', USERVER_NAMESPACE::chaotic::Maximum'
                f'<{type_name_ns}::k{type_name_decl}Maximum>'
            )

        if self.validators.exclusiveMin is not None:
            validators += (
                ', USERVER_NAMESPACE::chaotic::ExclusiveMinimum'
                f'<{type_name_ns}::k{type_name_decl}ExclusiveMinimum>'
            )
        if self.validators.exclusiveMax is not None:
            validators += (
                ', USERVER_NAMESPACE::chaotic::ExclusiveMaximum'
                f'<{type_name_ns}::k{type_name_decl}ExclusiveMaximum>'
            )

        if self.validators.minLength is not None:
            validators += (
                ', USERVER_NAMESPACE::chaotic::MinLength'
                f'<{self.validators.minLength}>'
            )
        if self.validators.maxLength is not None:
            validators += (
                ', USERVER_NAMESPACE::chaotic::MaxLength'
                f'<{self.validators.maxLength}>'
            )

        if self.validators.pattern is not None:
            validators += (
                ', USERVER_NAMESPACE::chaotic::Pattern'
                f'<{type_name_ns}::k{type_name_decl}Pattern>'
            )

        parser_type = (
            f'USERVER_NAMESPACE::chaotic::Primitive<{self.raw_cpp_type.in_global_scope()}'
            f'{validators}>'
        )
        if self.user_cpp_type:
            return (
                f'USERVER_NAMESPACE::chaotic::WithType<{parser_type}, '
                f'{self.cpp_user_name()}>'
            )
        else:
            return parser_type

    def need_using_type(self) -> bool:
        return True

    def need_operator_lshift(self) -> bool:
        return False


@dataclasses.dataclass
class CppStringWithFormat(CppType):
    default: Optional[Any] = None
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
            cpp_type = (
                'USERVER_NAMESPACE::' + cpp_type[len(USERVER_COLONCOLON) :]
            )
        return cpp_type

    def declaration_includes(self) -> List[str]:
        includes = []
        if self.user_cpp_type:
            includes += self.get_include_by_cpp_type(self.user_cpp_type)
        includes += self.get_include_by_cpp_type(self.format_cpp_type)

        includes.append('string')
        return includes

    def definition_includes(self) -> List[str]:
        includes = ['userver/chaotic/primitive.hpp']
        includes.append('userver/chaotic/with_type.hpp')
        return includes

    def parser_type(self, ns: str, name: str) -> str:
        format_cpp_type = self.format_cpp_type
        if format_cpp_type.startswith(USERVER_COLONCOLON):
            format_cpp_type = (
                'USERVER_NAMESPACE::'
                + format_cpp_type[len(USERVER_COLONCOLON) :]
            )
        parser_type = f'USERVER_NAMESPACE::chaotic::Primitive<{self.raw_cpp_type.in_global_scope()}>'
        parser_type = (
            f'USERVER_NAMESPACE::chaotic::WithType<{parser_type}, '
            f'{format_cpp_type}>'
        )
        if self.user_cpp_type:
            parser_type = (
                f'USERVER_NAMESPACE::chaotic::WithType<{parser_type}, '
                f'{self.cpp_user_name()}>'
            )
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
    cpp_name: Optional[str] = None

    KNOWN_X_PROPERTIES = ['x-usrv-cpp-indirect', 'x-taxi-cpp-indirect']

    __hash__ = CppType.__hash__

    @property
    def default(self) -> str:
        return self.orig_cpp_type.default  # type: ignore

    def _cpp_name(self) -> str:
        if self.indirect:
            return (
                'USERVER_NAMESPACE::utils::'
                f'Box<{self.orig_cpp_type._cpp_name()}>'
            )
        else:
            return self.orig_cpp_type._cpp_name()

    def cpp_user_name(self) -> str:
        if self.indirect:
            return (
                'USERVER_NAMESPACE::utils::'
                f'Box<{self.orig_cpp_type.cpp_user_name()}>'
            )
        else:
            if not self.cpp_name or (
                    self.orig_cpp_type.cpp_user_name()
                    != self.orig_cpp_type.cpp_global_name()
            ):
                # x-usrv-cpp-type
                return self.orig_cpp_type.cpp_user_name()
            else:
                return self.cpp_name

    def declaration_includes(self) -> List[str]:
        if self.indirect:
            return ['userver/utils/box.hpp']
        if self.self_ref:
            return []
        else:
            return self.orig_cpp_type.declaration_includes()

    def definition_includes(self) -> List[str]:
        if self.indirect:
            return ['userver/chaotic/ref.hpp']
        if self.self_ref:
            return []
        else:
            return self.orig_cpp_type.definition_includes()

    def parser_type(self, ns: str, name: str) -> str:
        if self.indirect:
            return (
                'USERVER_NAMESPACE::chaotic::Ref'
                f'<{self.orig_cpp_type.parser_type(ns, name)}>'
            )
        return self.orig_cpp_type.parser_type(ns, name)

    def need_using_type(self) -> bool:
        return True

    def need_operator_lshift(self) -> bool:
        return False


def camel_case(string: str, no_lower_casing: bool = False) -> str:
    result = ''
    set_upper = True
    for char in string:
        if char in {'_', '-', '.'}:
            set_upper = True
        else:
            if set_upper:
                char = char.upper()
            elif not no_lower_casing:
                char = char.lower()
            result += char
            set_upper = False
    return result


class EnumItemName(str):
    pass


@dataclasses.dataclass
class CppIntEnum(CppType):
    name: str
    enums: List[int]

    __hash__ = CppType.__hash__

    def declaration_includes(self) -> List[str]:
        return []

    def definition_includes(self) -> List[str]:
        return [
            'cstdint',
            'fmt/format.h',
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

    def definition_includes(self) -> List[str]:
        return ['fmt/format.h']


@dataclasses.dataclass
class CppStringEnum(CppType):
    name: str
    enums: List[CppStringEnumItem]
    default: Optional[EnumItemName]

    __hash__ = CppType.__hash__

    def declaration_includes(self) -> List[str]:
        return ['string']

    def definition_includes(self) -> List[str]:
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
    user_cpp_type: Optional[str] = None
    default: Optional[Any] = None  # the type already checked at front stage


@dataclasses.dataclass
class CppStructField:
    name: str
    required: bool
    schema: CppType

    def without_json_schema(self) -> 'CppStructField':
        if self.schema:
            return dataclasses.replace(
                self, schema=self.schema.without_json_schema(),
            )
        else:
            return self

    def _default(self) -> Optional[str]:
        return getattr(self.schema, 'default', None)

    def get_default(self) -> str:
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
    fields: Dict[str, CppStructField]
    # 'None' means 'do not generate extra member'
    extra_type: Union[CppType, bool, None] = False
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

    def without_json_schema(self) -> 'CppType':
        tmp = super().without_json_schema()
        assert isinstance(tmp, CppStruct)

        tmp.fields = dict(
            map(
                lambda x: (x[0], x[1].without_json_schema()),
                self.fields.items(),
            ),
        )
        if isinstance(self.extra_type, CppType):
            tmp.extra_type = self.extra_type.without_json_schema()
        return tmp

    def _is_default_dict_candidate(self) -> bool:
        if len(self.fields) != 1:
            return False
        default = self.fields.get('__default__')
        if (
                default
                and isinstance(self.extra_type, CppType)
                and default.schema.is_isomorphic(self.extra_type)
        ):
            return True
        return False

    def _is_default_dict(self) -> bool:
        return (
            self.autodiscover_default_dict
            and self._is_default_dict_candidate()
        )

    def cpp_user_name(self) -> str:
        if self._is_default_dict():
            assert isinstance(self.extra_type, CppType)
            return (
                'USERVER_NAMESPACE::utils::DefaultDict'
                f'<{self.extra_type.cpp_user_name()}>'
            )
        return super().cpp_user_name()

    def parser_type(self, ns: str, name: str) -> str:
        parser_type = self._primitive_parser_type()
        if self._is_default_dict():
            assert isinstance(self.extra_type, CppType)
            dict_type = (
                'USERVER_NAMESPACE::utils::DefaultDict'
                f'<{self.extra_type.cpp_user_name()}>'
            )
            return (
                f'USERVER_NAMESPACE::chaotic::WithType<{parser_type}, '
                f'{dict_type}>'
            )
        return parser_type

    def subtypes(self) -> List[CppType]:
        types = [field.schema for field in self.fields.values()]
        if (
                isinstance(self.extra_type, CppType)
                and not self._is_default_dict()
        ):
            types.append(self.extra_type)
        return types

    def extra_container(self) -> str:
        assert self.json_schema
        kwargs = self.json_schema.x_properties
        return kwargs.get(
            'x-usrv-cpp-extra-type',
            kwargs.get('x-taxi-cpp-extra-type', 'std::unordered_map'),
        )

    def declaration_includes(self) -> List[str]:
        includes = ['optional']
        if self.user_cpp_type:
            includes += self.get_include_by_cpp_type(self.user_cpp_type)
        for field in self.fields.values():
            includes.extend(field.schema.declaration_includes())

        if self.extra_type:
            includes.append('string')
            if isinstance(self.extra_type, CppType):
                extra_container = self.extra_container()
                includes += self.get_include_by_cpp_type(extra_container)
                includes.extend(self.extra_type.declaration_includes())
            else:
                includes.append('userver/formats/json/value.hpp')

        if self._is_default_dict():
            includes.append('userver/utils/default_dict.hpp')
        return includes

    def definition_includes(self) -> List[str]:
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
            includes += self.get_include_by_cpp_type(
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
    minItems: Optional[int] = None
    maxItems: Optional[int] = None

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

    def without_json_schema(self) -> 'CppArray':
        tmp = super().without_json_schema()
        assert isinstance(tmp, CppArray)

        tmp.items = self.items.without_json_schema()
        return tmp

    def subtypes(self) -> List[CppType]:
        return [self.items]

    def parser_type(self, ns: str, name: str) -> str:
        validators = ''
        if self.validators.minItems is not None:
            validators += (
                ', USERVER_NAMESPACE::chaotic::'
                f'MinItems<{self.validators.minItems}>'
            )
        if self.validators.maxItems is not None:
            validators += (
                ', USERVER_NAMESPACE::chaotic::'
                f'MaxItems<{self.validators.maxItems}>'
            )

        parser_type = (
            'USERVER_NAMESPACE::chaotic::Array'
            f'<{self.items.parser_type(ns, name)}, '
            f'{self.container}<{self.items.cpp_user_name()}>{validators}>'
        )
        user_cpp_type = self.user_cpp_type
        if user_cpp_type:
            parser_type = (
                f'USERVER_NAMESPACE::chaotic::WithType<{parser_type}, '
                f'{user_cpp_type}>'
            )
        return parser_type

    def declaration_includes(self) -> List[str]:
        includes = (
            self.get_include_by_cpp_type(self.container)
            + self.items.declaration_includes()
        )
        if self.user_cpp_type:
            includes += self.get_include_by_cpp_type(self.user_cpp_type)
        return includes

    def definition_includes(self) -> List[str]:
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
    parents: List[CppType]

    KNOWN_X_PROPERTIES = ['x-usrv-cpp-type', 'x-taxi-cpp-type']

    __hash__ = CppType.__hash__

    def declaration_includes(self) -> List[str]:
        includes = []
        if self.user_cpp_type:
            includes += self.get_include_by_cpp_type(self.user_cpp_type)
        includes += flatten(
            [item.declaration_includes() for item in self.parents],
        )
        return includes

    def definition_includes(self) -> List[str]:
        return [
            'userver/formats/common/merge.hpp',
            'userver/chaotic/primitive.hpp',
        ] + flatten([item.definition_includes() for item in self.parents])

    def parser_type(self, ns: str, name: str) -> str:
        return self._primitive_parser_type()

    def subtypes(self) -> List[CppType]:
        return self.parents

    def need_dom_parser(self) -> bool:
        return True

    def need_serializer(self) -> bool:
        return True

    def need_operator_eq(self) -> bool:
        return True


@dataclasses.dataclass
class CppVariant(CppType):
    variants: List[CppType]

    KNOWN_X_PROPERTIES = ['x-usrv-cpp-type', 'x-taxi-cpp-type']

    __hash__ = CppType.__hash__

    def declaration_includes(self) -> List[str]:
        includes = []
        if self.user_cpp_type:
            includes += self.get_include_by_cpp_type(self.user_cpp_type)
        return (
            includes
            + ['variant', 'userver/formats/parse/variant.hpp']
            + flatten([item.declaration_includes() for item in self.variants])
        )

    def definition_includes(self) -> List[str]:
        return [
            'userver/formats/json/serialize_variant.hpp',
            'userver/chaotic/primitive.hpp',
            'userver/chaotic/variant.hpp',
        ] + flatten([item.definition_includes() for item in self.variants])

    def parser_type(self, ns: str, name: str) -> str:
        parsers = []
        for variant in self.variants:
            parsers.append(variant.parser_type(ns, name))
        parser_type = (
            f'USERVER_NAMESPACE::chaotic::Variant<{",".join(parsers)}>'
        )
        if self.user_cpp_type:
            return (
                f'USERVER_NAMESPACE::chaotic::WithType<{parser_type}, '
                f'{self.cpp_user_name()}>'
            )
        else:
            return parser_type

    def subtypes(self) -> List[CppType]:
        return self.variants

    def need_operator_lshift(self) -> bool:
        return False


@dataclasses.dataclass
class CppVariantWithDiscriminator(CppType):
    property_name: str
    variants: Dict[str, CppType]

    KNOWN_X_PROPERTIES = ['x-usrv-cpp-type', 'x-taxi-cpp-type']

    __hash__ = CppType.__hash__

    def declaration_includes(self) -> List[str]:
        includes = ['variant', 'userver/chaotic/oneof_with_discriminator.hpp']
        if self.user_cpp_type:
            includes += self.get_include_by_cpp_type(self.user_cpp_type)
        return includes + flatten(
            [item.declaration_includes() for item in self.variants.values()],
        )

    def definition_includes(self) -> List[str]:
        return ['userver/formats/json/serialize_variant.hpp'] + flatten(
            [item.definition_includes() for item in self.variants.values()],
        )

    def parser_type(self, ns: str, name: str) -> str:
        variants_list = []
        for _, variant in self.variants.items():
            variants_list.append(variant.parser_type(ns, name))
        variants = ', '.join(variants_list)

        settings_name = (
            self.raw_cpp_type.parent().joinns(
                'k' + self.cpp_local_name() + '_Settings',
            )
        ).in_global_scope()

        parser_type = (
            'USERVER_NAMESPACE::chaotic::OneOfWithDiscriminator'
            f'<&{settings_name}, {variants}>'
        )
        if self.user_cpp_type:
            return (
                f'USERVER_NAMESPACE::chaotic::WithType<{parser_type}, '
                f'{self.cpp_user_name()}>'
            )
        else:
            return parser_type

    def need_operator_lshift(self) -> bool:
        return False
