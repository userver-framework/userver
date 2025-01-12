from __future__ import annotations

__all__ = ["PropertyProtocol", "Value"]

from abc import abstractmethod
from dataclasses import dataclass
from typing import TYPE_CHECKING, Any, ClassVar, Protocol, TypeVar

from ... import Config
from ... import schema as oai
from ...utils import PythonIdentifier
from ..errors import ParseError, PropertyError

if TYPE_CHECKING:  # pragma: no cover
    from .model_property import ModelProperty
else:
    ModelProperty = "ModelProperty"


@dataclass
class Value:
    """
    Some literal values in OpenAPI documents (like defaults) have to be converted into Python code safely
    (with string escaping, for example). We still keep the `raw_value` around for merging `allOf`.
    """

    python_code: str
    raw_value: Any


PropertyType = TypeVar("PropertyType", bound="PropertyProtocol")


class PropertyProtocol(Protocol):
    """
    Describes a single property for a schema

    Attributes:
        template: Name of the template file (if any) to use for this property. Must be stored in
            templates/property_templates and must contain two macros: construct and transform. Construct will be used to
            build this property from JSON data (a response from an API). Transform will be used to convert this property
            to JSON data (when sending a request to the API).

    Raises:
        ValidationError: Raised when the default value fails to be converted to the expected type
    """

    name: str
    required: bool
    _type_string: ClassVar[str] = ""
    _json_type_string: ClassVar[str] = ""  # Type of the property after JSON serialization
    _allowed_locations: ClassVar[set[oai.ParameterLocation]] = {
        oai.ParameterLocation.QUERY,
        oai.ParameterLocation.PATH,
        oai.ParameterLocation.COOKIE,
    }
    default: Value | None
    python_name: PythonIdentifier
    description: str | None
    example: str | None

    template: ClassVar[str] = "any_property.py.jinja"
    json_is_dict: ClassVar[bool] = False

    @abstractmethod
    def convert_value(self, value: Any) -> Value | None | PropertyError:
        """Convert a string value to a Value object"""
        raise NotImplementedError()  # pragma: no cover

    def validate_location(self, location: oai.ParameterLocation) -> ParseError | None:
        """Returns an error if this type of property is not allowed in the given location"""
        if location not in self._allowed_locations:
            return ParseError(detail=f"{self.get_type_string()} is not allowed in {location}")
        if location == oai.ParameterLocation.PATH and not self.required:
            return ParseError(detail="Path parameter must be required")
        return None

    def set_python_name(self, new_name: str, config: Config, skip_snake_case: bool = False) -> None:
        """Mutates this Property to set a new python_name.

        Required to mutate due to how Properties are stored and the difficulty of updating them in-dict.
        `new_name` will be validated before it is set, so `python_name` is not guaranteed to equal `new_name` after
        calling this.
        """
        object.__setattr__(
            self,
            "python_name",
            PythonIdentifier(value=new_name, prefix=config.field_prefix, skip_snake_case=skip_snake_case),
        )

    def get_base_type_string(self, *, quoted: bool = False) -> str:
        """Get the string describing the Python type of this property. Base types no require quoting."""
        return f'"{self._type_string}"' if not self.is_base_type and quoted else self._type_string

    def get_base_json_type_string(self, *, quoted: bool = False) -> str:
        """Get the string describing the JSON type of this property. Base types no require quoting."""
        return f'"{self._json_type_string}"' if not self.is_base_type and quoted else self._json_type_string

    def get_type_string(
        self,
        no_optional: bool = False,
        json: bool = False,
        *,
        multipart: bool = False,
        quoted: bool = False,
    ) -> str:
        """
        Get a string representation of type that should be used when declaring this property

        Args:
            no_optional: Do not include Optional or Unset even if the value is optional (needed for isinstance checks)
            json: True if the type refers to the property after JSON serialization
            multipart: True if the type should be used in a multipart request
            quoted: True if the type should be wrapped in quotes (if not a base type)
        """
        if json:
            type_string = self.get_base_json_type_string(quoted=quoted)
        elif multipart:
            type_string = "tuple[None, bytes, str]"
        else:
            type_string = self.get_base_type_string(quoted=quoted)

        if no_optional or self.required:
            return type_string
        return f"Union[Unset, {type_string}]"

    def get_instance_type_string(self) -> str:
        """Get a string representation of runtime type that should be used for `isinstance` checks"""
        return self.get_type_string(no_optional=True, quoted=False)

    # noinspection PyUnusedLocal
    def get_imports(self, *, prefix: str) -> set[str]:
        """
        Get a set of import strings that should be included when this property is used somewhere

        Args:
            prefix: A prefix to put before any relative (local) module names. This should be the number of . to get
            back to the root of the generated client.
        """
        imports = set()
        if not self.required:
            imports.add("from typing import Union")
            imports.add(f"from {prefix}types import UNSET, Unset")
        return imports

    def get_lazy_imports(self, *, prefix: str) -> set[str]:
        """Get a set of lazy import strings that should be included when this property is used somewhere

        Args:
            prefix: A prefix to put before any relative (local) module names. This should be the number of . to get
            back to the root of the generated client.
        """
        return set()

    def to_string(self) -> str:
        """How this should be declared in a dataclass"""
        default: str | None
        if self.default is not None:
            default = self.default.python_code
        elif not self.required:
            default = "UNSET"
        else:
            default = None

        if default is not None:
            return f"{self.python_name}: {self.get_type_string(quoted=True)} = {default}"
        return f"{self.python_name}: {self.get_type_string(quoted=True)}"

    def to_docstring(self) -> str:
        """Returns property docstring"""
        doc = f"{self.python_name} ({self.get_type_string()}): {self.description or ''}"
        if self.default:
            doc += f" Default: {self.default.python_code}."
        if self.example:
            doc += f" Example: {self.example}."
        return doc

    @property
    def is_base_type(self) -> bool:
        """Base types, represented by any other of `Property` than `ModelProperty` should not be quoted."""
        from . import ListProperty, ModelProperty, UnionProperty

        return self.__class__.__name__ not in {
            ModelProperty.__name__,
            ListProperty.__name__,
            UnionProperty.__name__,
        }
