from __future__ import annotations

__all__ = [
    "AnyProperty",
    "Class",
    "EnumProperty",
    "LiteralEnumProperty",
    "ModelProperty",
    "Parameters",
    "Property",
    "Schemas",
    "build_parameters",
    "build_schemas",
    "property_from_data",
]

from collections.abc import Iterable

from attrs import evolve

from ... import Config, utils
from ... import schema as oai
from ..errors import ParameterError, ParseError, PropertyError
from .any import AnyProperty
from .boolean import BooleanProperty
from .const import ConstProperty
from .date import DateProperty
from .datetime import DateTimeProperty
from .enum_property import EnumProperty
from .file import FileProperty
from .float import FloatProperty
from .int import IntProperty
from .list_property import ListProperty
from .literal_enum_property import LiteralEnumProperty
from .model_property import ModelProperty, process_model
from .none import NoneProperty
from .property import Property
from .schemas import (
    Class,
    Parameters,
    ReferencePath,
    Schemas,
    parse_reference_path,
    update_parameters_with_data,
    update_schemas_with_data,
)
from .string import StringProperty
from .union import UnionProperty
from .uuid import UuidProperty


def _string_based_property(
    name: str, required: bool, data: oai.Schema, config: Config
) -> StringProperty | DateProperty | DateTimeProperty | FileProperty | UuidProperty | PropertyError:
    """Construct a Property from the type "string" """
    string_format = data.schema_format
    python_name = utils.PythonIdentifier(value=name, prefix=config.field_prefix)
    if string_format == "date-time":
        return DateTimeProperty.build(
            name=name,
            required=required,
            default=data.default,
            python_name=python_name,
            description=data.description,
            example=data.example,
        )
    if string_format == "date":
        return DateProperty.build(
            name=name,
            required=required,
            default=data.default,
            python_name=python_name,
            description=data.description,
            example=data.example,
        )
    if string_format == "binary":
        return FileProperty.build(
            name=name,
            required=required,
            default=None,
            python_name=python_name,
            description=data.description,
            example=data.example,
        )
    if string_format == "uuid":
        return UuidProperty.build(
            name=name,
            required=required,
            default=data.default,
            python_name=python_name,
            description=data.description,
            example=data.example,
        )
    return StringProperty.build(
        name=name,
        default=data.default,
        required=required,
        python_name=python_name,
        description=data.description,
        example=data.example,
    )


def _property_from_ref(
    name: str,
    required: bool,
    parent: oai.Schema | None,
    data: oai.Reference,
    schemas: Schemas,
    config: Config,
    roots: set[ReferencePath | utils.ClassName],
) -> tuple[Property | PropertyError, Schemas]:
    ref_path = parse_reference_path(data.ref)
    if isinstance(ref_path, ParseError):
        return PropertyError(data=data, detail=ref_path.detail), schemas
    existing = schemas.classes_by_reference.get(ref_path)
    if not existing:
        return (
            PropertyError(data=data, detail="Could not find reference in parsed models or enums"),
            schemas,
        )

    default = existing.convert_value(parent.default) if parent is not None else None
    if isinstance(default, PropertyError):
        default.data = parent or data
        return default, schemas

    prop = evolve(
        existing,
        required=required,
        name=name,
        python_name=utils.PythonIdentifier(value=name, prefix=config.field_prefix),
        default=default,  # type: ignore # mypy can't tell that default comes from the same class...
    )

    schemas.add_dependencies(ref_path=ref_path, roots=roots)
    return prop, schemas


def property_from_data(  # noqa: PLR0911, PLR0912
    name: str,
    required: bool,
    data: oai.Reference | oai.Schema,
    schemas: Schemas,
    parent_name: str,
    config: Config,
    process_properties: bool = True,
    roots: set[ReferencePath | utils.ClassName] | None = None,
) -> tuple[Property | PropertyError, Schemas]:
    """Generate a Property from the OpenAPI dictionary representation of it"""
    roots = roots or set()
    name = utils.remove_string_escapes(name)
    if isinstance(data, oai.Reference):
        return _property_from_ref(
            name=name,
            required=required,
            parent=None,
            data=data,
            schemas=schemas,
            config=config,
            roots=roots,
        )

    sub_data: list[oai.Schema | oai.Reference] = data.allOf + data.anyOf + data.oneOf
    # A union of a single reference should just be passed through to that reference (don't create copy class)
    if len(sub_data) == 1 and isinstance(sub_data[0], oai.Reference):
        prop, schemas = _property_from_ref(
            name=name,
            required=required,
            parent=data,
            data=sub_data[0],
            schemas=schemas,
            config=config,
            roots=roots,
        )
        # We won't be generating a separate Python class for this schema - references to it will just use
        # the class for the schema it's referencing - so we don't add it to classes_by_name; but we do
        # add it to models_to_process, if it's a model, because its properties still need to be resolved.
        if isinstance(prop, ModelProperty):
            schemas = evolve(
                schemas,
                models_to_process=[*schemas.models_to_process, prop],
            )
        return prop, schemas

    if data.type == oai.DataType.BOOLEAN:
        return (
            BooleanProperty.build(
                name=name,
                required=required,
                default=data.default,
                python_name=utils.PythonIdentifier(value=name, prefix=config.field_prefix),
                description=data.description,
                example=data.example,
            ),
            schemas,
        )
    if data.enum:
        if config.literal_enums:
            return LiteralEnumProperty.build(
                data=data,
                name=name,
                required=required,
                schemas=schemas,
                parent_name=parent_name,
                config=config,
            )
        return EnumProperty.build(
            data=data,
            name=name,
            required=required,
            schemas=schemas,
            parent_name=parent_name,
            config=config,
        )
    if data.anyOf or data.oneOf or isinstance(data.type, list):
        return UnionProperty.build(
            data=data,
            name=name,
            required=required,
            schemas=schemas,
            parent_name=parent_name,
            config=config,
        )
    if data.const is not None:
        return (
            ConstProperty.build(
                name=name,
                required=required,
                default=data.default,
                const=data.const,
                python_name=utils.PythonIdentifier(value=name, prefix=config.field_prefix),
                description=data.description,
            ),
            schemas,
        )
    if data.type == oai.DataType.STRING:
        return (
            _string_based_property(name=name, required=required, data=data, config=config),
            schemas,
        )
    if data.type == oai.DataType.NUMBER:
        return (
            FloatProperty.build(
                name=name,
                default=data.default,
                required=required,
                python_name=utils.PythonIdentifier(value=name, prefix=config.field_prefix),
                description=data.description,
                example=data.example,
            ),
            schemas,
        )
    if data.type == oai.DataType.INTEGER:
        return (
            IntProperty.build(
                name=name,
                default=data.default,
                required=required,
                python_name=utils.PythonIdentifier(value=name, prefix=config.field_prefix),
                description=data.description,
                example=data.example,
            ),
            schemas,
        )
    if data.type == oai.DataType.NULL:
        return (
            NoneProperty(
                name=name,
                required=required,
                default=None,
                python_name=utils.PythonIdentifier(value=name, prefix=config.field_prefix),
                description=data.description,
                example=data.example,
            ),
            schemas,
        )
    if data.type == oai.DataType.ARRAY:
        return ListProperty.build(
            data=data,
            name=name,
            required=required,
            schemas=schemas,
            parent_name=parent_name,
            config=config,
            process_properties=process_properties,
            roots=roots,
        )
    if data.type == oai.DataType.OBJECT or data.allOf or (data.type is None and data.properties):
        return ModelProperty.build(
            data=data,
            name=name,
            schemas=schemas,
            required=required,
            parent_name=parent_name,
            config=config,
            process_properties=process_properties,
            roots=roots,
        )
    return (
        AnyProperty.build(
            name=name,
            required=required,
            default=data.default,
            python_name=utils.PythonIdentifier(value=name, prefix=config.field_prefix),
            description=data.description,
            example=data.example,
        ),
        schemas,
    )


def _create_schemas(
    *,
    components: dict[str, oai.Reference | oai.Schema],
    schemas: Schemas,
    config: Config,
) -> Schemas:
    to_process: Iterable[tuple[str, oai.Reference | oai.Schema]] = components.items()
    still_making_progress = True
    errors: list[PropertyError] = []

    # References could have forward References so keep going as long as we are making progress
    while still_making_progress:
        still_making_progress = False
        errors = []
        next_round = []
        # Only accumulate errors from the last round, since we might fix some along the way
        for name, data in to_process:
            if isinstance(data, oai.Reference):
                schemas.errors.append(PropertyError(data=data, detail="Reference schemas are not supported."))
                continue
            ref_path = parse_reference_path(f"#/components/schemas/{name}")
            if isinstance(ref_path, ParseError):
                schemas.errors.append(PropertyError(detail=ref_path.detail, data=data))
                continue
            schemas_or_err = update_schemas_with_data(ref_path=ref_path, data=data, schemas=schemas, config=config)
            if isinstance(schemas_or_err, PropertyError):
                next_round.append((name, data))
                errors.append(schemas_or_err)
                continue
            schemas = schemas_or_err
            still_making_progress = True
        to_process = next_round

    schemas.errors.extend(errors)
    return schemas


def _propogate_removal(*, root: ReferencePath | utils.ClassName, schemas: Schemas, error: PropertyError) -> None:
    if isinstance(root, utils.ClassName):
        schemas.classes_by_name.pop(root, None)
        return
    if root in schemas.classes_by_reference:
        error.detail = error.detail or ""
        error.detail += f"\n{root}"
        del schemas.classes_by_reference[root]
        for child in schemas.dependencies.get(root, set()):
            _propogate_removal(root=child, schemas=schemas, error=error)


def _process_model_errors(
    model_errors: list[tuple[ModelProperty, PropertyError]], *, schemas: Schemas
) -> list[PropertyError]:
    for model, error in model_errors:
        error.detail = error.detail or ""
        error.detail += "\n\nFailure to process schema has resulted in the removal of:"
        for root in model.roots:
            _propogate_removal(root=root, schemas=schemas, error=error)
    return [error for _, error in model_errors]


def _process_models(*, schemas: Schemas, config: Config) -> Schemas:
    to_process = schemas.models_to_process
    still_making_progress = True
    final_model_errors: list[tuple[ModelProperty, PropertyError]] = []
    latest_model_errors: list[tuple[ModelProperty, PropertyError]] = []

    # Models which refer to other models in their allOf must be processed after their referenced models
    while still_making_progress:
        still_making_progress = False
        # Only accumulate errors from the last round, since we might fix some along the way
        latest_model_errors = []
        next_round = []
        for model_prop in to_process:
            schemas_or_err = process_model(model_prop, schemas=schemas, config=config)
            if isinstance(schemas_or_err, PropertyError):
                schemas_or_err.header = f"\nUnable to process schema {model_prop.name}:"
                if isinstance(schemas_or_err.data, oai.Reference) and schemas_or_err.data.ref.endswith(
                    f"/{model_prop.class_info.name}"
                ):
                    schemas_or_err.detail = schemas_or_err.detail or ""
                    schemas_or_err.detail += "\n\nRecursive allOf reference found"
                    final_model_errors.append((model_prop, schemas_or_err))
                    continue
                latest_model_errors.append((model_prop, schemas_or_err))
                next_round.append(model_prop)
                continue
            schemas = schemas_or_err
            still_making_progress = True
        to_process = next_round

    final_model_errors.extend(latest_model_errors)
    errors = _process_model_errors(final_model_errors, schemas=schemas)
    return evolve(schemas, errors=[*schemas.errors, *errors], models_to_process=to_process)


def build_schemas(
    *,
    components: dict[str, oai.Reference | oai.Schema],
    schemas: Schemas,
    config: Config,
) -> Schemas:
    """Get a list of Schemas from an OpenAPI dict"""
    schemas = _create_schemas(components=components, schemas=schemas, config=config)
    schemas = _process_models(schemas=schemas, config=config)
    return schemas


def build_parameters(
    *,
    components: dict[str, oai.Reference | oai.Parameter],
    parameters: Parameters,
    config: Config,
) -> Parameters:
    """Get a list of Parameters from an OpenAPI dict"""
    to_process: Iterable[tuple[str, oai.Reference | oai.Parameter]] = []
    if components is not None:
        to_process = components.items()
    still_making_progress = True
    errors: list[ParameterError] = []

    # References could have forward References so keep going as long as we are making progress
    while still_making_progress:
        still_making_progress = False
        errors = []
        next_round = []
        # Only accumulate errors from the last round, since we might fix some along the way
        for name, data in to_process:
            if isinstance(data, oai.Reference):
                parameters.errors.append(ParameterError(data=data, detail="Reference parameters are not supported."))
                continue
            ref_path = parse_reference_path(f"#/components/parameters/{name}")
            if isinstance(ref_path, ParseError):
                parameters.errors.append(ParameterError(detail=ref_path.detail, data=data))
                continue
            parameters_or_err = update_parameters_with_data(
                ref_path=ref_path, data=data, parameters=parameters, config=config
            )
            if isinstance(parameters_or_err, ParameterError):
                next_round.append((name, data))
                errors.append(parameters_or_err)
                continue
            parameters = parameters_or_err
            still_making_progress = True
        to_process = next_round

    parameters.errors.extend(errors)
    return parameters
