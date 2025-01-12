from __future__ import annotations

from typing import Any, ClassVar

from attr import define

from ... import schema as oai
from ...utils import PythonIdentifier
from ..errors import PropertyError
from .protocol import PropertyProtocol, Value


@define
class BooleanProperty(PropertyProtocol):
    """Property for bool"""

    name: str
    required: bool
    default: Value | None
    python_name: PythonIdentifier
    description: str | None
    example: str | None

    _type_string: ClassVar[str] = "bool"
    _json_type_string: ClassVar[str] = "bool"
    _allowed_locations: ClassVar[set[oai.ParameterLocation]] = {
        oai.ParameterLocation.QUERY,
        oai.ParameterLocation.PATH,
        oai.ParameterLocation.COOKIE,
        oai.ParameterLocation.HEADER,
    }
    template: ClassVar[str] = "boolean_property.py.jinja"

    @classmethod
    def build(
        cls,
        name: str,
        required: bool,
        default: Any,
        python_name: PythonIdentifier,
        description: str | None,
        example: str | None,
    ) -> BooleanProperty | PropertyError:
        checked_default = cls.convert_value(default)
        if isinstance(checked_default, PropertyError):
            return checked_default
        return cls(
            name=name,
            required=required,
            default=checked_default,
            python_name=python_name,
            description=description,
            example=example,
        )

    @classmethod
    def convert_value(cls, value: Any) -> Value | None | PropertyError:
        if isinstance(value, Value) or value is None:
            return value
        if isinstance(value, str):
            if value.lower() == "true":
                return Value(python_code="True", raw_value=value)
            elif value.lower() == "false":
                return Value(python_code="False", raw_value=value)
        if isinstance(value, bool):
            return Value(python_code=str(value), raw_value=value)
        return PropertyError(f"Invalid boolean value: {value}")
