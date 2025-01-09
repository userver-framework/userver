from __future__ import annotations

from typing import Any, ClassVar

from attr import define

from ...utils import PythonIdentifier
from .protocol import PropertyProtocol, Value


@define
class AnyProperty(PropertyProtocol):
    """A property that can be any type (used for empty schemas)"""

    @classmethod
    def build(
        cls,
        name: str,
        required: bool,
        default: Any,
        python_name: PythonIdentifier,
        description: str | None,
        example: str | None,
    ) -> AnyProperty:
        return cls(
            name=name,
            required=required,
            default=AnyProperty.convert_value(default),
            python_name=python_name,
            description=description,
            example=example,
        )

    @classmethod
    def convert_value(cls, value: Any) -> Value | None:
        from .string import StringProperty

        if value is None:
            return value
        if isinstance(value, str):
            return StringProperty.convert_value(value)
        return Value(python_code=str(value), raw_value=value)

    name: str
    required: bool
    default: Value | None
    python_name: PythonIdentifier
    description: str | None
    example: str | None
    _type_string: ClassVar[str] = "Any"
    _json_type_string: ClassVar[str] = "Any"
