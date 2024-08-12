import collections
import dataclasses
import os
import re
from typing import Callable
from typing import Dict
from typing import List
from typing import NoReturn
from typing import Optional

from chaotic import error
from chaotic.back.cpp import type_name
from chaotic.back.cpp import types as cpp_types
from chaotic.front import types


@dataclasses.dataclass
class GeneratorConfig:
    # vfull -> namespace
    namespaces: Dict[str, str]
    # infile_path -> cpp type
    infile_to_name_func: Optional[Callable] = None
    # type: ignore
    include_dirs: Optional[List[str]] = dataclasses.field(
        # type: ignore
        default_factory=list,
    )
    autodiscover_default_dict: bool = False
    strict_parsing_default: bool = True


@dataclasses.dataclass
class GeneratorState:
    types: Dict[str, cpp_types.CppType]
    refs: Dict[types.Schema, str]
    ref_objects: List[cpp_types.CppRef]
    external_types: Dict[types.Schema, cpp_types.CppType]


NON_NAME_SYMBOL_RE = re.compile('[^_0-9a-zA-Z]')


class FormatChooser:
    def __init__(self, types: List[cpp_types.CppType]) -> None:
        self.types = types
        self.parent: Dict[
            cpp_types.CppType, List[Optional[cpp_types.CppType]],
        ] = collections.defaultdict(list)

    def check_for_json_onlyness(self) -> None:
        def add(
                parent: Optional[cpp_types.CppType], type_: cpp_types.CppType,
        ) -> None:
            self.parent[type_].append(parent)
            for subtype in type_.subtypes():
                add(type_, subtype)

            if isinstance(type_, cpp_types.CppRef):
                self.parent[type_.orig_cpp_type].append(type_)

        for type_ in self.types:
            add(None, type_)

        def mark_as_only_json(type_: cpp_types.CppType, reason: str) -> None:
            assert reason
            # pylint: disable=protected-access
            type_._only_json_reason = reason

            for parent in self.parent[type_]:
                if not parent:
                    continue

                if parent._only_json_reason:
                    continue
                mark_as_only_json(parent, reason)

        for type_ in self.parent:
            if isinstance(type_, cpp_types.CppVariantWithDiscriminator):
                mark_as_only_json(
                    type_,
                    f'{type_.raw_cpp_type} has JSON-specific field "extra"',
                )
            if isinstance(type_, cpp_types.CppStruct):
                assert isinstance(type_, cpp_types.CppStruct)

                if type_.extra_type is True:
                    mark_as_only_json(
                        type_,
                        f'{type_.raw_cpp_type} has JSON-specific field "extra"',
                    )


class TranslatorError(error.BaseError):
    pass


class Generator:
    def __init__(self, config: GeneratorConfig) -> None:
        self._config = config
        self._state = GeneratorState(
            types=collections.OrderedDict(),
            refs={},
            ref_objects=[],
            external_types={},
        )
        self._state.ref_objects = []

    def generate_types(
            self,
            schemas: types.ResolvedSchemas,
            external_schemas: Dict[str, cpp_types.CppType] = {},
    ) -> Dict[str, cpp_types.CppType]:
        for cpp_type in external_schemas.values():
            schema = cpp_type.json_schema
            assert schema
            self._state.external_types[schema] = cpp_type

        for name, schema in schemas.schemas.items():
            fq_cpp_name = self._gen_fq_cpp_name(name)
            self._state.refs[schema] = fq_cpp_name
            self._state.types[fq_cpp_name] = self._generate_type(
                type_name.TypeName(fq_cpp_name), schema,
            )

        self.fixup_refs()
        self.fixup_formats()

        return self._state.types

    def _validate_type(self, type_: cpp_types.CppType) -> None:
        if not type_.user_cpp_type:
            return
        if type_.has_generated_user_cpp_type():
            return

        if self._config.include_dirs is None:
            # no check at all
            return

        user_include = type_.cpp_type_to_user_include_path(type_.user_cpp_type)
        for include_dir in self._config.include_dirs:
            path = os.path.join(include_dir, user_include)
            if os.path.exists(path):
                return

        assert type_.json_schema
        self._raise(
            type_.json_schema,
            msg=(
                f'Include file "{user_include}" not found, tried paths:\n'
                + '\n'.join(
                    [
                        '- ' + include_dir
                        for include_dir in self._config.include_dirs
                    ],
                )
            ),
        )

    def _raise(self, schema: types.Schema, msg: str) -> NoReturn:
        location = schema.source_location()
        raise TranslatorError(
            full_filepath=location.filepath,
            infile_path=location.location,
            schema_type='jsonschema',
            msg=msg,
        )

    def _validate_x_properties(self, type_: cpp_types.CppType) -> None:
        slots = type_.KNOWN_X_PROPERTIES
        assert type_.json_schema
        properties = type_.json_schema.x_properties
        for prop in properties:
            if (
                    not prop.startswith('x-usrv-cpp-')
                    and not prop.startswith('x-taxi-cpp-')
                    and not prop.startswith('x-cpp-')
            ):
                continue
            if prop in slots:
                continue

            source_location = type_.json_schema.source_location()
            file = source_location.filepath
            location = source_location.location
            self._raise(
                type_.json_schema,
                f'Unknown x- property: {prop} in {location} at {file}',
            )

    def _extract_user_cpp_type(self, schema: types.Schema) -> Optional[str]:
        cpp_type = schema.get_x_property_str(
            'x-usrv-cpp-type',
        ) or schema.get_x_property_str('x-taxi-cpp-type')
        if cpp_type:
            cpp_type = cpp_type.strip()
        return cpp_type or None

    def _extract_container(self, schema: types.Schema) -> str:
        container = schema.get_x_property_str(
            'x-usrv-cpp-container', 'std::vector',
        )
        assert container is not None
        return container

    def fixup_refs(self) -> None:
        for ref in self._state.ref_objects:
            assert isinstance(ref.json_schema, types.Ref)
            schema = ref.json_schema.schema
            name = self._state.refs.get(schema)
            if name:
                orig_cpp_type = self._state.types[name]
            else:
                orig_cpp_type = self._state.external_types[schema]

            ref.orig_cpp_type = orig_cpp_type
            ref.indirect = ref.json_schema.indirect
            ref.self_ref = ref.json_schema.self_ref
            ref.cpp_name = name

    def fixup_formats(self) -> None:
        chooser = FormatChooser(list(self._state.types.values()))
        chooser.check_for_json_onlyness()

    def _generate_type(
            self, fq_cpp_name: type_name.TypeName, schema: types.Schema,
    ) -> cpp_types.CppType:
        method = SCHEMA_GENERATORS[type(schema)]
        cpp_type = method(self, fq_cpp_name, schema)  # type: ignore
        self._validate_type(cpp_type)
        self._validate_x_properties(cpp_type)
        return cpp_type

    def _gen_fq_cpp_name(self, jsonschema_name: str) -> str:
        vfile, infile = jsonschema_name.split('#')
        if self._config.infile_to_name_func:
            name = self._config.infile_to_name_func(infile)
        else:
            name = infile
        namespace = self._config.namespaces[vfile]
        if namespace:
            return namespace + '::' + name
        else:
            return name

    def _gen_boolean(
            self, name: type_name.TypeName, schema: types.Boolean,
    ) -> cpp_types.CppType:

        return cpp_types.CppPrimitiveType(
            json_schema=schema,
            nullable=schema.nullable,
            raw_cpp_type=type_name.TypeName('bool'),
            user_cpp_type=self._extract_user_cpp_type(schema),
            validators=cpp_types.CppPrimitiveValidator(),
            default=schema.default,
        )

    def _gen_integer(
            self, name: type_name.TypeName, schema: types.Integer,
    ) -> cpp_types.CppType:
        user_cpp_type = self._extract_user_cpp_type(schema)

        if schema.enum:
            assert schema.format is None
            assert user_cpp_type is None

            return cpp_types.CppIntEnum(
                json_schema=schema,
                nullable=schema.nullable,
                raw_cpp_type=name,
                user_cpp_type=None,
                name=name.in_global_scope(),
                enums=schema.enum,
            )

        if schema.format is None:
            raw_cpp_type = 'int'
        elif schema.format.value == 32:
            raw_cpp_type = 'std::int32_t'
        elif schema.format.value == 64:
            raw_cpp_type = 'std::int64_t'
        else:
            self._raise(
                schema, f'"format: {schema.format.value}" is not implemented',
            )

        typedef_tag = schema.get_x_property_str(
            'x-usrv-cpp-typedef-tag',
        ) or schema.get_x_property_str('x-taxi-cpp-typedef-tag')
        if typedef_tag:
            if user_cpp_type:
                self._raise(
                    schema,
                    '"x-usrv-cpp-typedef-tag" and "x-usrv-cpp-type" are mutually exclusive',
                )
            user_cpp_type = (
                f'userver::utils::StrongTypedef<{typedef_tag}, {raw_cpp_type}>'
            )

        validators = cpp_types.CppPrimitiveValidator(
            min=schema.minimum,
            max=schema.maximum,
            exclusiveMin=schema.exclusiveMinimum,
            exclusiveMax=schema.exclusiveMaximum,
            namespace=name.namespace(),
            prefix=name.in_local_scope(),
        )
        validators.fix_legacy_exclusive()
        return cpp_types.CppPrimitiveType(
            json_schema=schema,
            nullable=schema.nullable,
            raw_cpp_type=type_name.TypeName(raw_cpp_type),
            user_cpp_type=user_cpp_type,
            validators=validators,
            default=schema.default,
        )

    def _gen_number(
            self, name: type_name.TypeName, schema: types.Number,
    ) -> cpp_types.CppType:
        user_cpp_type = self._extract_user_cpp_type(schema)

        validators = cpp_types.CppPrimitiveValidator(
            min=schema.minimum,
            max=schema.maximum,
            exclusiveMin=schema.exclusiveMinimum,
            exclusiveMax=schema.exclusiveMaximum,
            namespace=name.namespace(),
            prefix=name.in_local_scope(),
        )
        validators.fix_legacy_exclusive()
        return cpp_types.CppPrimitiveType(
            json_schema=schema,
            nullable=schema.nullable,
            raw_cpp_type=type_name.TypeName('double'),
            user_cpp_type=user_cpp_type,
            validators=validators,
            default=schema.default,
        )

    def _str_enum_name(self, item: str) -> str:
        cpp_name = cpp_types.camel_case(self._normalize_name(item))
        if cpp_name[0].isnumeric():
            cpp_name = 'X' + cpp_name
        return 'k' + cpp_name

    def _gen_string(
            self, name: type_name.TypeName, schema: types.String,
    ) -> cpp_types.CppType:
        user_cpp_type = self._extract_user_cpp_type(schema)

        if schema.enum:
            assert not user_cpp_type

            enums = []
            for item in schema.enum:
                enums.append(
                    cpp_types.CppStringEnumItem(
                        raw_name=item, cpp_name=self._str_enum_name(item),
                    ),
                )

            default: Optional[cpp_types.EnumItemName]
            if schema.default:
                default = cpp_types.EnumItemName(
                    name.in_global_scope()
                    + '::'
                    + self._str_enum_name(schema.default),
                )
            else:
                default = None

            return cpp_types.CppStringEnum(
                json_schema=schema,
                nullable=schema.nullable,
                raw_cpp_type=name,
                user_cpp_type=None,
                name=name.in_global_scope(),
                enums=enums,
                default=default,
            )

        validators = cpp_types.CppPrimitiveValidator(
            minLength=schema.minLength,
            maxLength=schema.maxLength,
            pattern=schema.pattern,
            namespace=name.namespace(),
            prefix=name.in_local_scope(),
        )

        typedef_tag = schema.get_x_property_str(
            'x-usrv-cpp-typedef-tag',
        ) or schema.get_x_property_str('x-taxi-cpp-typedef-tag')
        if typedef_tag:
            if user_cpp_type:
                self._raise(
                    schema,
                    '"x-usrv-cpp-typedef-tag" and "x-usrv-cpp-type" are mutually exclusive',
                )
            user_cpp_type = (
                f'userver::utils::StrongTypedef<{typedef_tag}, std::string>'
            )

        if schema.format:
            if schema.format == types.StringFormat.UUID:
                format_cpp_type = 'boost::uuids::uuid'
            elif schema.format == types.StringFormat.DATE:
                format_cpp_type = 'userver::utils::datetime::Date'
            elif schema.format in [
                types.StringFormat.DATE_TIME,
                types.StringFormat.DATE_TIME_ISO_BASIC,
            ]:
                if schema.format == types.StringFormat.DATE_TIME:
                    format_cpp_type = 'userver::utils::datetime::TimePointTz'
                elif schema.format == types.StringFormat.DATE_TIME_ISO_BASIC:
                    format_cpp_type = (
                        'userver::utils::datetime::TimePointTzIsoBasic'
                    )
                else:
                    self._raise(
                        schema, f'Using unknown "format: {schema.format}"',
                    )
            else:
                self._raise(
                    schema, f'"format: {schema.format}" is unsupported yet',
                )
            return cpp_types.CppStringWithFormat(
                json_schema=schema,
                nullable=schema.nullable,
                raw_cpp_type=type_name.TypeName('std::string'),
                user_cpp_type=user_cpp_type,
                default=schema.default,
                format_cpp_type=format_cpp_type,
            )

        return cpp_types.CppPrimitiveType(
            json_schema=schema,
            nullable=schema.nullable,
            raw_cpp_type=type_name.TypeName('std::string'),
            user_cpp_type=user_cpp_type,
            validators=validators,
            default=schema.default,
        )

    @staticmethod
    def _normalize_name(name: str) -> str:
        return re.sub(NON_NAME_SYMBOL_RE, '_', name)

    def _gen_field(
            self,
            class_name: type_name.TypeName,
            field_name: str,
            schema: types.Schema,
            required: bool,
    ) -> cpp_types.CppStructField:
        # Field name must not be the same as the type
        type_name = field_name.title()
        if type_name == field_name:
            type_name = type_name + '_'

        # Struct X may not have subtype X
        if type_name == class_name.in_local_scope():
            type_name = type_name + '_'

        type_name = self._normalize_name(type_name)

        cpp_name = schema.get_x_property_str(
            'x-usrv-cpp-name',
        ) or schema.get_x_property_str('x-taxi-cpp-name')
        schema.delete_x_property('x-usrv-cpp-name')
        schema.delete_x_property('x-taxi-cpp-name')

        name = class_name.joinns(type_name)
        type_ = self._generate_type(name, schema)

        field = cpp_types.CppStructField(
            name=cpp_name or self._normalize_name(field_name),
            required=required,
            schema=type_,
        )

        return field

    def _gen_array(
            self, name: type_name.TypeName, schema: types.Array,
    ) -> cpp_types.CppType:
        # TODO: name?
        items = self._generate_type(name.add_suffix('A'), schema.items)

        user_cpp_type = self._extract_user_cpp_type(schema)
        container = self._extract_container(schema)

        if (
                container == 'std::vector'
                and user_cpp_type in LEGACY_X_TAXI_CPP_TYPE_CONTAINERS
        ):
            container = user_cpp_type
            user_cpp_type = None

        return cpp_types.CppArray(
            # _cpp_type() is overridden in array
            raw_cpp_type=type_name.TypeName('NOT_USED'),
            json_schema=schema,
            nullable=schema.nullable,
            user_cpp_type=user_cpp_type,
            items=items,
            container=container,
            validators=cpp_types.CppArrayValidator(
                minItems=schema.minItems, maxItems=schema.maxItems,
            ),
        )

    def _gen_all_of(
            self, name: type_name.TypeName, schema: types.AllOf,
    ) -> cpp_types.CppType:
        parents = []
        for num, subtype in enumerate(schema.allOf):
            parents.append(
                self._generate_type(name.add_suffix(f'__P{num}'), subtype),
            )

        user_cpp_type = self._extract_user_cpp_type(schema)

        return cpp_types.CppStructAllOf(
            raw_cpp_type=name,
            user_cpp_type=user_cpp_type,
            json_schema=schema,
            nullable=schema.nullable,
            parents=parents,
        )

    def _gen_one_of(
            self,
            name: type_name.TypeName,
            schema: types.OneOfWithoutDiscriminator,
    ) -> cpp_types.CppType:
        variants = []
        for num, subtype in enumerate(schema.oneOf):
            variants.append(
                self._generate_type(name.add_suffix(f'__O{num}'), subtype),
            )

        user_cpp_type = self._extract_user_cpp_type(schema)

        obj = cpp_types.CppVariant(
            raw_cpp_type=name,
            user_cpp_type=user_cpp_type,
            json_schema=schema,
            nullable=schema.nullable,
            variants=variants,
        )
        return obj

    def _gen_one_of_with_discriminator(
            self,
            name: type_name.TypeName,
            schema: types.OneOfWithDiscriminator,
    ) -> cpp_types.CppType:
        variants = {}
        for item in zip(schema.oneOf, schema.mapping):
            field_value, refs = item
            for ref in refs:
                variants[ref] = self._gen_ref(
                    type_name.TypeName(''), field_value,
                )

        assert schema.discriminator_property

        user_cpp_type = self._extract_user_cpp_type(schema)

        return cpp_types.CppVariantWithDiscriminator(
            raw_cpp_type=name,
            user_cpp_type=user_cpp_type,
            json_schema=schema,
            nullable=schema.nullable,
            variants=variants,
            property_name=schema.discriminator_property,
        )

    def _gen_object(
            self, name: type_name.TypeName, schema: types.SchemaObject,
    ) -> cpp_types.CppType:
        assert schema.properties is not None
        required = schema.required or []

        fields = {
            field_name: self._gen_field(
                name, field_name, schema, required=field_name in required,
            )
            for field_name, schema in schema.properties.items()
        }

        extra_type: cpp_types.CppType | bool | None
        if schema.additionalProperties:
            if isinstance(schema.additionalProperties, types.Schema):
                extra_name = 'Extra'
                if extra_name == name.in_local_scope():
                    extra_name += '_'

                type_ = self._generate_type(
                    name.joinns(extra_name), schema.additionalProperties,
                )
                extra_type = type_
            else:
                assert schema.additionalProperties is True
                extra_type = True
        else:
            extra_type = False

        user_cpp_type = self._extract_user_cpp_type(schema)

        need_extra_member = schema.get_x_property_bool(
            'x-usrv-cpp-extra-member',
            schema.get_x_property_bool('x-taxi-cpp-extra-member', True),
        )
        if not need_extra_member and not isinstance(extra_type, bool):
            self._raise(
                schema,
                msg=(
                    '"x-usrv-cpp-extra-member: false" is not allowed for non-boolean '
                    '"additionalProperties"'
                ),
            )
        if not need_extra_member:
            extra_type = None

        strict_parsing = schema.get_x_property_bool(
            'x-taxi-strict-parsing', self._config.strict_parsing_default,
        )

        return cpp_types.CppStruct(
            raw_cpp_type=name,
            user_cpp_type=user_cpp_type,
            json_schema=schema,
            nullable=schema.nullable,
            fields=fields,
            extra_type=extra_type,
            autodiscover_default_dict=self._config.autodiscover_default_dict,
            strict_parsing=strict_parsing,
        )

    def _gen_ref(
            self, name: type_name.TypeName, schema: types.Ref,
    ) -> cpp_types.CppType:
        ref = cpp_types.CppRef(
            json_schema=schema,
            nullable=False,
            # stub
            user_cpp_type=None,
            orig_cpp_type=cpp_types.CppType(
                raw_cpp_type=type_name.TypeName(''),
                json_schema=None,
                nullable=False,
                user_cpp_type=None,
            ),
            raw_cpp_type=type_name.TypeName(''),
            indirect=False,
            self_ref=False,
        )
        self._state.ref_objects.append(ref)
        return ref


# pylint: disable=protected-access
SCHEMA_GENERATORS = {
    types.Boolean: Generator._gen_boolean,
    types.Integer: Generator._gen_integer,
    types.Number: Generator._gen_number,
    types.String: Generator._gen_string,
    types.SchemaObject: Generator._gen_object,
    types.Array: Generator._gen_array,
    types.Ref: Generator._gen_ref,
    types.AllOf: Generator._gen_all_of,
    types.OneOfWithoutDiscriminator: Generator._gen_one_of,
    types.OneOfWithDiscriminator: Generator._gen_one_of_with_discriminator,
}

LEGACY_X_TAXI_CPP_TYPE_CONTAINERS = (
    'std::set',
    'std::unordered_set',
    'std::vector',
)
