from __future__ import annotations

from typing import Any, ClassVar

from attr import define

from ...utils import PythonIdentifier
from ..errors import PropertyError
from .protocol import PropertyProtocol


@define
class FileProperty(PropertyProtocol):
    """A property used for uploading files"""

    name: str
    required: bool
    default: None
    python_name: PythonIdentifier
    description: str | None
    example: str | None

    _type_string: ClassVar[str] = "File"
    # Return type of File.to_tuple()
    _json_type_string: ClassVar[str] = "FileJsonType"
    template: ClassVar[str] = "file_property.py.jinja"

    @classmethod
    def build(
        cls,
        name: str,
        required: bool,
        default: Any,
        python_name: PythonIdentifier,
        description: str | None,
        example: str | None,
    ) -> FileProperty | PropertyError:
        default_or_err = cls.convert_value(default)
        if isinstance(default_or_err, PropertyError):
            return default_or_err

        return cls(
            name=name,
            required=required,
            default=default_or_err,
            python_name=python_name,
            description=description,
            example=example,
        )

    @classmethod
    def convert_value(cls, value: Any) -> None | PropertyError:
        if value is not None:
            return PropertyError(detail="File properties cannot have a default value")
        return value

    def get_imports(self, *, prefix: str) -> set[str]:
        """
        Get a set of import strings that should be included when this property is used somewhere

        Args:
            prefix: A prefix to put before any relative (local) module names. This should be the number of . to get
            back to the root of the generated client.
        """
        imports = super().get_imports(prefix=prefix)
        imports.update({f"from {prefix}types import File, FileJsonType", "from io import BytesIO"})
        return imports
