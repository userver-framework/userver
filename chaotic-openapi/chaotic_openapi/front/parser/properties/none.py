from __future__ import annotations

from typing import Any, ClassVar

from attr import define

from ... import schema as oai
from ...utils import PythonIdentifier
from ..errors import PropertyError
from .protocol import PropertyProtocol, Value


@define
class NoneProperty(PropertyProtocol):
    """A property that can only be None"""

    name: str
    required: bool
    default: Value | None
    python_name: PythonIdentifier
    description: str | None
    example: str | None

    _allowed_locations: ClassVar[set[oai.ParameterLocation]] = {
        oai.ParameterLocation.QUERY,
        oai.ParameterLocation.COOKIE,
        oai.ParameterLocation.HEADER,
    }
    _type_string: ClassVar[str] = "None"
    _json_type_string: ClassVar[str] = "None"

    @classmethod
    def build(
        cls,
        name: str,
        required: bool,
        default: Any,
        python_name: PythonIdentifier,
        description: str | None,
        example: str | None,
    ) -> NoneProperty | PropertyError:
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
        if value is None or isinstance(value, Value):
            return value
        if isinstance(value, str):
            if value == "None":
                return Value(python_code=value, raw_value=value)
        return PropertyError(f"Value {value} is not valid, only None is allowed")
