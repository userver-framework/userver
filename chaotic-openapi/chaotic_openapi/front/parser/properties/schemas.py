__all__ = [
    "Class",
    "Parameters",
    "ReferencePath",
    "Schemas",
    "parameter_from_data",
    "parameter_from_reference",
    "parse_reference_path",
    "update_parameters_with_data",
    "update_schemas_with_data",
]

from typing import TYPE_CHECKING, NewType, Union, cast
from urllib.parse import urlparse

from attrs import define, evolve, field

from ... import Config
from ... import schema as oai
from ...schema.openapi_schema_pydantic import Parameter
from ...utils import ClassName, PythonIdentifier
from ..errors import ParameterError, ParseError, PropertyError

if TYPE_CHECKING:  # pragma: no cover
    from .model_property import ModelProperty
    from .property import Property
else:
    ModelProperty = "ModelProperty"
    Property = "Property"


ReferencePath = NewType("ReferencePath", str)


def parse_reference_path(ref_path_raw: str) -> Union[ReferencePath, ParseError]:
    """
    Takes a raw string provided in a `$ref` and turns it into a validated `_ReferencePath` or a `ParseError` if
    validation fails.

    See Also:
        - https://swagger.io/docs/specification/using-ref/
    """
    parsed = urlparse(ref_path_raw)
    if parsed.scheme or parsed.path:
        return ParseError(detail=f"Remote references such as {ref_path_raw} are not supported yet.")
    return cast(ReferencePath, parsed.fragment)


@define
class Class:
    """Represents Python class which will be generated from an OpenAPI schema"""

    name: ClassName
    module_name: PythonIdentifier

    @staticmethod
    def from_string(*, string: str, config: Config) -> "Class":
        """Get a Class from an arbitrary string"""
        class_name = string.split("/")[-1]  # Get rid of ref path stuff
        class_name = ClassName(class_name, config.field_prefix)
        override = config.class_overrides.get(class_name)

        if override is not None and override.class_name is not None:
            class_name = ClassName(override.class_name, config.field_prefix)

        if override is not None and override.module_name is not None:
            module_name = override.module_name
        else:
            module_name = class_name
        module_name = PythonIdentifier(module_name, config.field_prefix)

        return Class(name=class_name, module_name=module_name)


@define
class Schemas:
    """Structure for containing all defined, shareable, and reusable schemas (attr classes and Enums)"""

    classes_by_reference: dict[ReferencePath, Property] = field(factory=dict)
    dependencies: dict[ReferencePath, set[Union[ReferencePath, ClassName]]] = field(factory=dict)
    classes_by_name: dict[ClassName, Property] = field(factory=dict)
    models_to_process: list[ModelProperty] = field(factory=list)
    errors: list[ParseError] = field(factory=list)

    def add_dependencies(self, ref_path: ReferencePath, roots: set[Union[ReferencePath, ClassName]]) -> None:
        """Record new dependencies on the given ReferencePath

        Args:
            ref_path: The ReferencePath being referenced
            roots: A set of identifiers for the objects dependent on the object corresponding to `ref_path`
        """
        self.dependencies.setdefault(ref_path, set())
        self.dependencies[ref_path].update(roots)


def update_schemas_with_data(
    *, ref_path: ReferencePath, data: oai.Schema, schemas: Schemas, config: Config
) -> Union[Schemas, PropertyError]:
    """
    Update a `Schemas` using some new reference.

    Args:
        ref_path: The output of `parse_reference_path` (validated $ref).
        data: The schema of the thing to add to Schemas.
        schemas: `Schemas` up until now.
        config: User-provided config for overriding default behavior.

    Returns:
        Either the updated `schemas` input or a `PropertyError` if something went wrong.

    See Also:
        - https://swagger.io/docs/specification/using-ref/
    """
    from . import property_from_data

    prop: Union[PropertyError, Property]
    prop, schemas = property_from_data(
        data=data,
        name=ref_path,
        schemas=schemas,
        required=True,
        parent_name="",
        config=config,
        # Don't process ModelProperty properties because schemas are still being created
        process_properties=False,
        roots={ref_path},
    )

    if isinstance(prop, PropertyError):
        prop.detail = f"{prop.header}: {prop.detail}"
        prop.header = f"Unable to parse schema {ref_path}"
        if isinstance(prop.data, oai.Reference) and prop.data.ref.endswith(ref_path):  # pragma: nocover
            prop.detail += (
                "\n\nRecursive and circular references are not supported directly in an array schema's 'items' section"
            )
        return prop

    schemas = evolve(schemas, classes_by_reference={ref_path: prop, **schemas.classes_by_reference})
    return schemas


@define
class Parameters:
    """Structure for containing all defined, shareable, and reusable parameters"""

    classes_by_reference: dict[ReferencePath, Parameter] = field(factory=dict)
    classes_by_name: dict[ClassName, Parameter] = field(factory=dict)
    errors: list[ParseError] = field(factory=list)


def parameter_from_data(
    *,
    name: str,
    data: Union[oai.Reference, oai.Parameter],
    parameters: Parameters,
    config: Config,
) -> tuple[Union[Parameter, ParameterError], Parameters]:
    """Generates parameters from an OpenAPI Parameter spec."""

    if isinstance(data, oai.Reference):
        return ParameterError("Unable to resolve another reference"), parameters

    if data.param_schema is None:
        return ParameterError("Parameter has no schema"), parameters

    new_param = Parameter(
        name=name,
        required=data.required,
        explode=data.explode,
        style=data.style,
        param_schema=data.param_schema,
        param_in=data.param_in,
    )
    parameters = evolve(
        parameters, classes_by_name={**parameters.classes_by_name, ClassName(name, config.field_prefix): new_param}
    )
    return new_param, parameters


def update_parameters_with_data(
    *, ref_path: ReferencePath, data: oai.Parameter, parameters: Parameters, config: Config
) -> Union[Parameters, ParameterError]:
    """
    Update a `Parameters` using some new reference.

    Args:
        ref_path: The output of `parse_reference_path` (validated $ref).
        data: The schema of the thing to add to Schemas.
        parameters: `Parameters` up until now.

    Returns:
        Either the updated `parameters` input or a `PropertyError` if something went wrong.

    See Also:
        - https://swagger.io/docs/specification/using-ref/
    """
    param, parameters = parameter_from_data(data=data, name=data.name, parameters=parameters, config=config)

    if isinstance(param, ParameterError):
        param.detail = f"{param.header}: {param.detail}"
        param.header = f"Unable to parse parameter {ref_path}"
        if isinstance(param.data, oai.Reference) and param.data.ref.endswith(ref_path):  # pragma: nocover
            param.detail += (
                "\n\nRecursive and circular references are not supported. "
                "See https://github.com/openapi-generators/openapi-python-client/issues/466"
            )
        return param

    parameters = evolve(parameters, classes_by_reference={ref_path: param, **parameters.classes_by_reference})
    return parameters


def parameter_from_reference(
    *,
    param: Union[oai.Reference, Parameter],
    parameters: Parameters,
) -> Union[Parameter, ParameterError]:
    """
    Returns a Parameter from a Reference or the Parameter itself if one was provided.

    Args:
        param: A parameter by `Reference`.
        parameters: `Parameters` up until now.

    Returns:
        Either the updated `schemas` input or a `PropertyError` if something went wrong.

    See Also:
        - https://swagger.io/docs/specification/using-ref/
    """
    if isinstance(param, Parameter):
        return param

    ref_path = parse_reference_path(param.ref)

    if isinstance(ref_path, ParseError):
        return ParameterError(detail=ref_path.detail)

    _resolved_parameter_class = parameters.classes_by_reference.get(ref_path, None)
    if _resolved_parameter_class is None:
        return ParameterError(detail=f"Reference `{ref_path}` not found.")
    return _resolved_parameter_class
