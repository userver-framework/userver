from __future__ import annotations

from typing import Any, ClassVar, overload

from attr import define

from ...utils import PythonIdentifier
from ..errors import PropertyError
from .protocol import PropertyProtocol, Value
from .string import StringProperty


@define
class ConstProperty(PropertyProtocol):
    """A property representing a const value"""

    name: str
    required: bool
    value: Value
    default: Value | None
    python_name: PythonIdentifier
    description: str | None
    example: None
    template: ClassVar[str] = "const_property.py.jinja"

    @classmethod
    def build(
        cls,
        *,
        const: str | int | float | bool,
        default: Any,
        name: str,
        python_name: PythonIdentifier,
        required: bool,
        description: str | None,
    ) -> ConstProperty | PropertyError:
        """
        Create a `ConstProperty` the right way.

        Args:
            const: The `const` value of the schema, indicating the literal value this represents
            default: The default value of this property, if any. Must be equal to `const` if set.
            name: The name of the property where it appears in the OpenAPI document.
            required: Whether this property is required where it's being used.
            python_name: The name used to represent this variable/property in generated Python code
            description: The description of this property, used for docstrings
        """
        value = cls._convert_value(const)

        prop = cls(
            value=value,
            python_name=python_name,
            name=name,
            required=required,
            default=None,
            description=description,
            example=None,
        )
        converted_default = prop.convert_value(default)
        if isinstance(converted_default, PropertyError):
            return converted_default
        prop.default = converted_default
        return prop

    def convert_value(self, value: Any) -> Value | None | PropertyError:
        value = self._convert_value(value)
        if value is None:
            return value
        if value != self.value:
            return PropertyError(
                detail=f"Invalid value for const {self.name}; {value.raw_value} != {self.value.raw_value}"
            )
        return value

    @staticmethod
    @overload
    def _convert_value(value: None) -> None:  # type: ignore[misc]
        ...  # pragma: no cover

    @staticmethod
    @overload
    def _convert_value(value: Any) -> Value: ...  # pragma: no cover

    @staticmethod
    def _convert_value(value: Any) -> Value | None:
        if value is None or isinstance(value, Value):
            return value
        if isinstance(value, str):
            return StringProperty.convert_value(value)
        return Value(python_code=str(value), raw_value=value)

    def get_type_string(
        self,
        no_optional: bool = False,
        json: bool = False,
        *,
        multipart: bool = False,
        quoted: bool = False,
    ) -> str:
        lit = f"Literal[{self.value.python_code}]"
        if not no_optional and not self.required:
            return f"Union[{lit}, Unset]"
        return lit

    def get_imports(self, *, prefix: str) -> set[str]:
        """
        Get a set of import strings that should be included when this property is used somewhere

        Args:
            prefix: A prefix to put before any relative (local) module names. This should be the number of . to get
            back to the root of the generated client.
        """
        if self.required:
            return {"from typing import Literal, cast"}
        return {
            "from typing import Literal, Union, cast",
            f"from {prefix}types import UNSET, Unset",
        }
