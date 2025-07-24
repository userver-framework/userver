import collections
import dataclasses
import enum
import typing
from typing import Any
from typing import Callable
from typing import Dict
from typing import List
from typing import Optional
from typing import TypeVar
from typing import Union

from chaotic import error


class FieldError(Exception):
    def __init__(self, field: str, msg: str) -> None:
        self.field = field
        self.msg = msg


def is_ignored_prefix(arg: str) -> bool:
    if arg.startswith('x-'):
        return True
    if arg in ('description', 'example'):
        return True
    return False


def smart_fields(cls: type) -> type:
    orig_init = cls.__init__  # type: ignore

    def __init__(self, **kwargs) -> None:
        known_fields = {field.name for field in dataclasses.fields(self) if field.init}
        ignored = {}

        # 1. for known pattern x-* field just ignore it
        # 2. for unknown field raise an exception with a specific text
        for arg in kwargs:
            if arg not in known_fields:
                if is_ignored_prefix(arg):
                    ignored[arg] = kwargs[arg]
                else:
                    raise FieldError(
                        arg,
                        f'Unknown field: "{arg}", known fields: ["' + '", "'.join(sorted(known_fields)) + '"]',
                    )

        for ignore in ignored:
            kwargs.pop(ignore)

        orig_init(self, **kwargs)
        self.x_properties = ignored

    cls.__init__ = __init__  # type: ignore
    return cls


def validate_type(field_name: str, value, type_) -> None:
    try:
        pytype = type_.__origin__
    except AttributeError:
        pytype = type_
    # pylint: disable=protected-access
    if isinstance(pytype, typing._SpecialForm):
        try:
            pytype = type_.__args__
        except AttributeError:
            pytype = type_

    try:
        if not isinstance(value, pytype):
            # TODO: better text
            # raise FieldError(field_name, f'{value} is not {pytype}')
            raise FieldError(
                field_name,
                f'field "{field_name}" has wrong type',
            )
    except TypeError:
        # TODO: type=List[str]
        pass


@dataclasses.dataclass(frozen=True)
class SourceLocation:
    # e.g. some/file/name.yaml
    filepath: str
    # e.g. /definitions/Type
    location: str

    def __str__(self) -> str:
        return f'{self.filepath}#{self.location}'


@dataclasses.dataclass
class Base:
    def __post_init__(self) -> None:
        for field in dataclasses.fields(self):
            validate_type(field.name, getattr(self, field.name), field.type)


_OptionalBool = TypeVar('_OptionalBool', bool, Optional[bool])
_OptionalStr = TypeVar('_OptionalStr', str, Optional[str])


@dataclasses.dataclass
class Schema(Base):
    x_properties: Dict[str, Any] = dataclasses.field(
        init=False,
        default_factory=dict,
    )

    def visit_children(self, cb: Callable[['Schema', 'Schema'], None]) -> None:
        pass

    def __hash__(self) -> int:
        return id(self)

    def source_location(self) -> SourceLocation:
        return self._source_location  # type: ignore

    def delete_x_property(self, name: str) -> None:
        if name in self.x_properties:
            del self.x_properties[name]

    def get_x_property_str(
        self,
        name: str,
        default: _OptionalStr = None,
    ) -> _OptionalStr:
        return self._get_x_property_typed(name, default, str, 'string')

    def get_x_property_bool(
        self,
        name: str,
        default: _OptionalBool = None,
    ) -> _OptionalBool:
        return self._get_x_property_typed(name, default, bool, 'boolean')

    def _get_x_property_typed(
        self,
        name: str,
        default: Any,
        type_: type,
        type_str: str,
    ) -> Any:
        value = self.x_properties.get(name, default)
        if value is None:
            return None

        if isinstance(value, type_):
            return value

        raise error.BaseError(
            full_filepath=self.source_location().filepath,
            infile_path=self.source_location().location,
            schema_type='jsonschema',
            msg=f'Non-{type_str} x- property "{name}"',
        )


_NOT_IMPL = Schema()


@dataclasses.dataclass
class Ref(Schema):
    ref: str  # type: ignore
    indirect: bool
    self_ref: bool
    schema: Schema = _NOT_IMPL

    def __post_init__(self):
        assert self.ref.find('/../') == -1

    __hash__ = Schema.__hash__


@smart_fields
@dataclasses.dataclass
class Boolean(Schema):
    type: str = 'boolean'
    default: Optional[bool] = None
    nullable: bool = False

    __hash__ = Schema.__hash__


# TODO: default & nullable => raise


class IntegerFormat(enum.Enum):
    INT32 = 32
    INT64 = 64

    @classmethod
    def from_string(cls, data: str) -> 'IntegerFormat':
        fmt = INTEGER_FORMAT_TO_FORMAT.get(data)
        if not fmt:
            raise Exception(f'Unknown format: {data}')
        return fmt


INTEGER_FORMAT_TO_FORMAT = {
    'int32': IntegerFormat.INT32,
    'int64': IntegerFormat.INT64,
}


@smart_fields
@dataclasses.dataclass
class Integer(Schema):
    type: str = 'integer'
    default: Optional[int] = None
    nullable: bool = False
    minimum: Optional[int] = None
    maximum: Optional[int] = None
    exclusiveMinimum: Optional[int] = None
    exclusiveMaximum: Optional[int] = None
    # TODO: multipleOf
    enum: Optional[List[int]] = None
    format: Optional[IntegerFormat] = None

    def __post_init__(self) -> None:
        super().__post_init__()
        if self.enum:
            for item in self.enum:
                if not isinstance(item, int):
                    raise FieldError(
                        'enum',
                        f'field "enum" contains non-integers ({item})',
                    )

    __hash__ = Schema.__hash__


@smart_fields
@dataclasses.dataclass
class Number(Schema):
    type: str = 'number'
    default: Optional[Union[float, int]] = None
    nullable: bool = False
    minimum: Optional[Union[float, int]] = None
    maximum: Optional[Union[float, int]] = None
    exclusiveMinimum: Optional[Union[float, int]] = None
    exclusiveMaximum: Optional[Union[float, int]] = None
    format: Optional[str] = None
    # TODO: multipleOf

    __hash__ = Schema.__hash__


class StringFormat(enum.Enum):
    BINARY = enum.auto()
    BYTE = enum.auto()
    DATE = enum.auto()
    DATE_TIME = enum.auto()
    DATE_TIME_ISO_BASIC = enum.auto()
    DATE_TIME_FRACTION = enum.auto()
    UUID = enum.auto()

    @classmethod
    def from_string(cls, data: str) -> 'StringFormat':
        fmt = STRING_FORMAT_TO_FORMAT.get(data)
        if not fmt:
            raise Exception(f'Unknown format: {data}')
        return fmt


STRING_FORMAT_TO_FORMAT = {
    'binary': StringFormat.BINARY,
    'byte': StringFormat.BYTE,
    'date': StringFormat.DATE,
    'date-time': StringFormat.DATE_TIME,
    'date-time-iso-basic': StringFormat.DATE_TIME_ISO_BASIC,
    'date-time-fraction': StringFormat.DATE_TIME_FRACTION,
    'uuid': StringFormat.UUID,
}


@smart_fields
@dataclasses.dataclass
class String(Schema):
    type: str = 'string'
    default: Optional[str] = None
    nullable: bool = False
    enum: Optional[List[str]] = None
    pattern: Optional[str] = None
    format: Optional[StringFormat] = None
    minLength: Optional[int] = None
    maxLength: Optional[int] = None

    def __post_init__(self) -> None:
        super().__post_init__()
        if self.enum:
            for item in self.enum:
                if not isinstance(item, str):
                    raise FieldError(
                        'enum',
                        f'field "enum" contains non-strings ({item})',
                    )

    __hash__ = Schema.__hash__


@smart_fields
@dataclasses.dataclass
class Array(Schema):
    # validated in SchemaParser._parse_array()
    items: Schema
    type: str = 'array'
    nullable: bool = False
    minItems: Optional[int] = None
    maxItems: Optional[int] = None

    def visit_children(self, cb: Callable[[Schema, Schema], None]) -> None:
        cb(self.items, self)
        self.items.visit_children(cb)

    __hash__ = Schema.__hash__


@smart_fields
@dataclasses.dataclass
class SchemaObjectRaw:
    type: str
    additionalProperties: Any
    properties: Optional[dict] = None
    required: Optional[List[str]] = None
    nullable: bool = False


@smart_fields
@dataclasses.dataclass
class SchemaObject(Schema):
    additionalProperties: Union[Schema, bool]
    properties: Dict[str, Schema]
    required: Optional[List[str]] = None
    nullable: bool = False

    def __post_init__(self) -> None:
        super().__post_init__()
        if self.required:
            for name in self.required:
                if name not in self.properties:
                    raise FieldError(
                        'required',
                        f'Field "{name}" is set in "required", but missing in "properties"',
                    )

    def visit_children(self, cb: Callable[[Schema, Schema], None]) -> None:
        if isinstance(self.additionalProperties, Schema):
            cb(self.additionalProperties, self)
            self.additionalProperties.visit_children(cb)

        for prop in self.properties.values():
            cb(prop, self)
            prop.visit_children(cb)

    __hash__ = Schema.__hash__


@smart_fields
@dataclasses.dataclass
class AllOf(Schema):
    allOf: List[Schema]  # type: ignore
    nullable: bool = False

    def visit_children(self, cb: Callable[[Schema, Schema], None]) -> None:
        for parent in self.allOf:
            cb(parent, self)
            parent.visit_children(cb)

    __hash__ = Schema.__hash__


@smart_fields
@dataclasses.dataclass
class AllOfRaw:
    allOf: list  # type:ignore
    nullable: bool = False

    def __post_init__(self) -> None:
        if not self.allOf:
            raise FieldError('allOf', 'Empty allOf')


@smart_fields
@dataclasses.dataclass
class OneOfWithoutDiscriminator(Schema):
    oneOf: List[Schema]  # type:ignore
    nullable: bool = False

    def visit_children(self, cb: Callable[[Schema, Schema], None]) -> None:
        for variant in self.oneOf:
            cb(variant, self)
            variant.visit_children(cb)

    __hash__ = Schema.__hash__


class MappingType(enum.Enum):
    STR = 0
    INT = 1


@dataclasses.dataclass
class DiscMapping:
    # only one list must be not none
    str_values: Optional[List[List[str]]] = None
    int_values: Optional[List[List[int]]] = None

    def append(self, value: list):
        if self.str_values is not None:
            self.str_values.append(typing.cast(List[str], value))
        elif self.int_values is not None:
            self.int_values.append(typing.cast(List[int], value))

    def enable_str(self):
        self.str_values = []
        self.int_values = None

    def enable_int(self):
        self.int_values = []
        self.str_values = None

    def get_type(self) -> MappingType:
        if self.is_int():
            return MappingType.INT

        return MappingType.STR

    def as_strs(self) -> List[List[str]]:
        return typing.cast(List[List[str]], self.str_values)

    def as_ints(self) -> List[List[int]]:
        return typing.cast(List[List[int]], self.int_values)

    def is_int(self):
        return self.int_values is not None

    def is_str(self):
        return self.str_values is not None


@smart_fields
@dataclasses.dataclass
class OneOfWithDiscriminator(Schema):
    oneOf: List[Ref]  # type:ignore
    discriminator_property: Optional[str] = None
    mapping: DiscMapping = dataclasses.field(default_factory=DiscMapping)
    nullable: bool = False

    def visit_children(self, cb: Callable[[Schema, Schema], None]) -> None:
        for variant in self.oneOf:
            cb(variant, self)
            variant.visit_children(cb)

    __hash__ = Schema.__hash__


@smart_fields
@dataclasses.dataclass
class OneOfDiscriminatorRaw:
    propertyName: str  # type:ignore
    mapping: Optional[Dict[str, str]] = None


@smart_fields
@dataclasses.dataclass
class OneOfRaw:
    oneOf: list  # type:ignore
    discriminator: Optional[dict] = None

    def __post_init__(self) -> None:
        if not self.oneOf:
            raise FieldError('oneOf', 'Empty oneOf')


@dataclasses.dataclass
class ParsedSchemas:
    schemas: Dict[str, Schema] = dataclasses.field(
        default_factory=collections.OrderedDict,
    )

    @staticmethod
    def merge(schemas: List['ParsedSchemas']) -> 'ParsedSchemas':
        result = ParsedSchemas()
        for schema in schemas:
            result.schemas.update(schema.schemas)
        return result


@dataclasses.dataclass
class ResolvedSchemas:
    schemas: Dict[str, Schema]
