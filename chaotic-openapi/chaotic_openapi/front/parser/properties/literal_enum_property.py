from __future__ import annotations

__all__ = ["LiteralEnumProperty"]

from typing import Any, ClassVar, Union, cast

from attr import evolve
from attrs import define

from ... import Config, utils
from ... import schema as oai
from ...schema import DataType
from ..errors import PropertyError
from .none import NoneProperty
from .protocol import PropertyProtocol, Value
from .schemas import Class, Schemas
from .union import UnionProperty

ValueType = Union[str, int]


@define
class LiteralEnumProperty(PropertyProtocol):
    """A property that should use a literal enum"""

    name: str
    required: bool
    default: Value | None
    python_name: utils.PythonIdentifier
    description: str | None
    example: str | None
    values: set[ValueType]
    class_info: Class
    value_type: type[ValueType]

    template: ClassVar[str] = "literal_enum_property.py.jinja"

    _allowed_locations: ClassVar[set[oai.ParameterLocation]] = {
        oai.ParameterLocation.QUERY,
        oai.ParameterLocation.PATH,
        oai.ParameterLocation.COOKIE,
        oai.ParameterLocation.HEADER,
    }

    @classmethod
    def build(  # noqa: PLR0911
        cls,
        *,
        data: oai.Schema,
        name: str,
        required: bool,
        schemas: Schemas,
        parent_name: str,
        config: Config,
    ) -> tuple[LiteralEnumProperty | NoneProperty | UnionProperty | PropertyError, Schemas]:
        """
        Create a LiteralEnumProperty from schema data.

        Args:
            data: The OpenAPI Schema which defines this enum.
            name: The name to use for variables which receive this Enum's value (e.g. model property name)
            required: Whether or not this Property is required in the calling context
            schemas: The Schemas which have been defined so far (used to prevent naming collisions)
            parent_name: The context in which this LiteralEnumProperty is defined, used to create more specific class names.
            config: The global config for this run of the generator

        Returns:
            A tuple containing either the created property or a PropertyError AND update schemas.
        """

        enum = data.enum or []  # The outer function checks for this, but mypy doesn't know that

        # OpenAPI allows for null as an enum value, but it doesn't make sense with how enums are constructed in Python.
        # So instead, if null is a possible value, make the property nullable.
        # Mypy is not smart enough to know that the type is right though
        unchecked_value_list = [value for value in enum if value is not None]  # type: ignore

        # It's legal to have an enum that only contains null as a value, we don't bother constructing an enum for that
        if len(unchecked_value_list) == 0:
            return (
                NoneProperty.build(
                    name=name,
                    required=required,
                    default="None",
                    python_name=utils.PythonIdentifier(value=name, prefix=config.field_prefix),
                    description=None,
                    example=None,
                ),
                schemas,
            )

        value_types = {type(value) for value in unchecked_value_list}
        if len(value_types) > 1:
            return PropertyError(
                header="Enum values must all be the same type", detail=f"Got {value_types}", data=data
            ), schemas
        value_type = next(iter(value_types))
        if value_type not in (str, int):
            return PropertyError(header=f"Unsupported enum type {value_type}", data=data), schemas
        value_list = cast(
            Union[list[int], list[str]], unchecked_value_list
        )  # We checked this with all the value_types stuff

        if len(value_list) < len(enum):  # Only one of the values was None, that becomes a union
            data.oneOf = [
                oai.Schema(type=DataType.NULL),
                data.model_copy(update={"enum": value_list, "default": data.default}),
            ]
            data.enum = None
            return UnionProperty.build(
                data=data,
                name=name,
                required=required,
                schemas=schemas,
                parent_name=parent_name,
                config=config,
            )

        class_name = data.title or name
        if parent_name:
            class_name = f"{utils.pascal_case(parent_name)}{utils.pascal_case(class_name)}"
        class_info = Class.from_string(string=class_name, config=config)
        values: set[str | int] = set(value_list)

        if class_info.name in schemas.classes_by_name:
            existing = schemas.classes_by_name[class_info.name]
            if not isinstance(existing, LiteralEnumProperty) or values != existing.values:
                return (
                    PropertyError(
                        detail=f"Found conflicting enums named {class_info.name} with incompatible values.", data=data
                    ),
                    schemas,
                )

        prop = LiteralEnumProperty(
            name=name,
            required=required,
            class_info=class_info,
            values=values,
            value_type=value_type,
            default=None,
            python_name=utils.PythonIdentifier(value=name, prefix=config.field_prefix),
            description=data.description,
            example=data.example,
        )
        checked_default = prop.convert_value(data.default)
        if isinstance(checked_default, PropertyError):
            checked_default.data = data
            return checked_default, schemas
        prop = evolve(prop, default=checked_default)

        schemas = evolve(schemas, classes_by_name={**schemas.classes_by_name, class_info.name: prop})
        return prop, schemas

    def convert_value(self, value: Any) -> Value | PropertyError | None:
        if value is None or isinstance(value, Value):
            return value
        if isinstance(value, self.value_type):
            if value in self.values:
                return Value(python_code=repr(value), raw_value=value)
            else:
                return PropertyError(detail=f"Value {value} is not valid for enum {self.name}")
        return PropertyError(detail=f"Cannot convert {value} to enum {self.name} of type {self.value_type}")

    def get_base_type_string(self, *, quoted: bool = False) -> str:
        return self.class_info.name

    def get_base_json_type_string(self, *, quoted: bool = False) -> str:
        return self.value_type.__name__

    def get_instance_type_string(self) -> str:
        return self.value_type.__name__

    def get_imports(self, *, prefix: str) -> set[str]:
        """
        Get a set of import strings that should be included when this property is used somewhere

        Args:
            prefix: A prefix to put before any relative (local) module names. This should be the number of . to get
            back to the root of the generated client.
        """
        imports = super().get_imports(prefix=prefix)
        imports.add("from typing import cast")
        imports.add(f"from {prefix}models.{self.class_info.module_name} import {self.class_info.name}")
        imports.add(
            f"from {prefix}models.{self.class_info.module_name} import check_{self.get_class_name_snake_case()}"
        )
        return imports

    def get_class_name_snake_case(self) -> str:
        return utils.snake_case(self.class_info.name)
