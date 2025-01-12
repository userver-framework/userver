from __future__ import annotations

from itertools import chain
from typing import Any, ClassVar, NamedTuple

from attrs import define, evolve

from ... import Config, utils
from ... import schema as oai
from ...utils import PythonIdentifier
from ..errors import ParseError, PropertyError
from .any import AnyProperty
from .protocol import PropertyProtocol, Value
from .schemas import Class, ReferencePath, Schemas, parse_reference_path


@define
class ModelProperty(PropertyProtocol):
    """A property which refers to another Schema"""

    name: str
    required: bool
    default: Value | None
    python_name: utils.PythonIdentifier
    example: str | None
    class_info: Class
    data: oai.Schema
    description: str
    roots: set[ReferencePath | utils.ClassName]
    required_properties: list[Property] | None
    optional_properties: list[Property] | None
    relative_imports: set[str] | None
    lazy_imports: set[str] | None
    additional_properties: Property | None
    _json_type_string: ClassVar[str] = "dict[str, Any]"

    template: ClassVar[str] = "model_property.py.jinja"
    json_is_dict: ClassVar[bool] = True
    is_multipart_body: bool = False

    @classmethod
    def build(
        cls,
        *,
        data: oai.Schema,
        name: str,
        schemas: Schemas,
        required: bool,
        parent_name: str | None,
        config: Config,
        process_properties: bool,
        roots: set[ReferencePath | utils.ClassName],
    ) -> tuple[ModelProperty | PropertyError, Schemas]:
        """
        A single ModelProperty from its OAI data

        Args:
            data: Data of a single Schema
            name: Name by which the schema is referenced, such as a model name.
                Used to infer the type name if a `title` property is not available.
            schemas: Existing Schemas which have already been processed (to check name conflicts)
            required: Whether or not this property is required by the parent (affects typing)
            parent_name: The name of the property that this property is inside of (affects class naming)
            config: Config data for this run of the generator, used to modifying names
            roots: Set of strings that identify schema objects on which the new ModelProperty will depend
            process_properties: Determines whether the new ModelProperty will be initialized with property data
        """
        if not config.use_path_prefixes_for_title_model_names and data.title:
            class_string = data.title
        else:
            title = data.title or name
            if parent_name:
                class_string = f"{utils.pascal_case(parent_name)}{utils.pascal_case(title)}"
            else:
                class_string = title
        class_info = Class.from_string(string=class_string, config=config)
        model_roots = {*roots, class_info.name}
        required_properties: list[Property] | None = None
        optional_properties: list[Property] | None = None
        relative_imports: set[str] | None = None
        lazy_imports: set[str] | None = None
        additional_properties: Property | None = None
        if process_properties:
            data_or_err, schemas = _process_property_data(
                data=data, schemas=schemas, class_info=class_info, config=config, roots=model_roots
            )
            if isinstance(data_or_err, PropertyError):
                return data_or_err, schemas
            property_data, additional_properties = data_or_err
            required_properties = property_data.required_props
            optional_properties = property_data.optional_props
            relative_imports = property_data.relative_imports
            lazy_imports = property_data.lazy_imports
            for root in roots:
                if isinstance(root, utils.ClassName):
                    continue
                schemas.add_dependencies(root, {class_info.name})

        prop = ModelProperty(
            class_info=class_info,
            data=data,
            roots=model_roots,
            required_properties=required_properties,
            optional_properties=optional_properties,
            relative_imports=relative_imports,
            lazy_imports=lazy_imports,
            additional_properties=additional_properties,
            description=data.description or "",
            default=None,
            required=required,
            name=name,
            python_name=utils.PythonIdentifier(value=name, prefix=config.field_prefix),
            example=data.example,
        )
        if class_info.name in schemas.classes_by_name:
            error = PropertyError(
                data=data, detail=f'Attempted to generate duplicate models with name "{class_info.name}"'
            )
            return error, schemas

        schemas = evolve(
            schemas,
            classes_by_name={**schemas.classes_by_name, class_info.name: prop},
            models_to_process=[*schemas.models_to_process, prop],
        )
        return prop, schemas

    @classmethod
    def convert_value(cls, value: Any) -> Value | None | PropertyError:
        if value is not None:
            return PropertyError(detail="ModelProperty cannot have a default value")  # pragma: no cover
        return None

    def __attrs_post_init__(self) -> None:
        if self.relative_imports:
            self.set_relative_imports(self.relative_imports)

    @property
    def self_import(self) -> str:
        """Constructs a self import statement from this ModelProperty's attributes"""
        return f"models.{self.class_info.module_name} import {self.class_info.name}"

    def get_base_type_string(self, *, quoted: bool = False) -> str:
        return f'"{self.class_info.name}"' if quoted else self.class_info.name

    def get_imports(self, *, prefix: str) -> set[str]:
        """
        Get a set of import strings that should be included when this property is used somewhere

        Args:
            prefix: A prefix to put before any relative (local) module names. This should be the number of . to get
            back to the root of the generated client.
        """
        imports = super().get_imports(prefix=prefix)
        imports.update(
            {
                "from typing import cast",
            }
        )
        return imports

    def get_lazy_imports(self, *, prefix: str) -> set[str]:
        """Get a set of lazy import strings that should be included when this property is used somewhere

        Args:
            prefix: A prefix to put before any relative (local) module names. This should be the number of . to get
            back to the root of the generated client.
        """
        return {f"from {prefix}{self.self_import}"}

    def set_relative_imports(self, relative_imports: set[str]) -> None:
        """Set the relative imports set for this ModelProperty, filtering out self imports

        Args:
            relative_imports: The set of relative import strings
        """
        object.__setattr__(self, "relative_imports", {ri for ri in relative_imports if self.self_import not in ri})

    def set_lazy_imports(self, lazy_imports: set[str]) -> None:
        """Set the lazy imports set for this ModelProperty, filtering out self imports

        Args:
            lazy_imports: The set of lazy import strings
        """
        object.__setattr__(self, "lazy_imports", {li for li in lazy_imports if self.self_import not in li})

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
        """
        if json:
            type_string = self.get_base_json_type_string()
        elif multipart:
            type_string = "tuple[None, bytes, str]"
        else:
            type_string = self.get_base_type_string()

        if quoted:
            if type_string == self.class_info.name:
                type_string = f"'{type_string}'"

        if no_optional or self.required:
            return type_string
        return f"Union[Unset, {type_string}]"


from .property import Property  # noqa: E402


def _resolve_naming_conflict(first: Property, second: Property, config: Config) -> PropertyError | None:
    first.set_python_name(first.name, config=config, skip_snake_case=True)
    second.set_python_name(second.name, config=config, skip_snake_case=True)
    if first.python_name == second.python_name:
        return PropertyError(
            header="Conflicting property names",
            detail=f"Properties {first.name} and {second.name} have the same python_name",
        )
    return None


class _PropertyData(NamedTuple):
    optional_props: list[Property]
    required_props: list[Property]
    relative_imports: set[str]
    lazy_imports: set[str]
    schemas: Schemas


def _process_properties(  # noqa: PLR0912, PLR0911
    *,
    data: oai.Schema,
    schemas: Schemas,
    class_name: utils.ClassName,
    config: Config,
    roots: set[ReferencePath | utils.ClassName],
) -> _PropertyData | PropertyError:
    from . import property_from_data
    from .merge_properties import merge_properties

    properties: dict[str, Property] = {}
    relative_imports: set[str] = set()
    lazy_imports: set[str] = set()
    required_set = set(data.required or [])

    def _add_if_no_conflict(new_prop: Property) -> PropertyError | None:
        nonlocal properties

        name_conflict = properties.get(new_prop.name)
        merged_prop = merge_properties(name_conflict, new_prop) if name_conflict else new_prop
        if isinstance(merged_prop, PropertyError):
            merged_prop.header = f"Found conflicting properties named {new_prop.name} when creating {class_name}"
            return merged_prop

        for other_prop in properties.values():
            if other_prop.name == merged_prop.name:
                continue  # Same property, probably just got merged
            if other_prop.python_name != merged_prop.python_name:
                continue
            naming_error = _resolve_naming_conflict(merged_prop, other_prop, config)
            if naming_error is not None:
                return naming_error

        properties[merged_prop.name] = merged_prop
        return None

    unprocessed_props: list[tuple[str, oai.Reference | oai.Schema]] = (
        list(data.properties.items()) if data.properties else []
    )
    for sub_prop in data.allOf:
        if isinstance(sub_prop, oai.Reference):
            ref_path = parse_reference_path(sub_prop.ref)
            if isinstance(ref_path, ParseError):
                return PropertyError(detail=ref_path.detail, data=sub_prop)
            sub_model = schemas.classes_by_reference.get(ref_path)
            if sub_model is None:
                return PropertyError(f"Reference {sub_prop.ref} not found")
            if not isinstance(sub_model, ModelProperty):
                return PropertyError("Cannot take allOf a non-object")
            # Properties of allOf references first should be processed first
            if not (
                isinstance(sub_model.required_properties, list) and isinstance(sub_model.optional_properties, list)
            ):
                return PropertyError(f"Reference {sub_model.name} in allOf was not processed", data=sub_prop)
            for prop in chain(sub_model.required_properties, sub_model.optional_properties):
                err = _add_if_no_conflict(prop)
                if err is not None:
                    return err
            schemas.add_dependencies(ref_path=ref_path, roots=roots)
        else:
            unprocessed_props.extend(sub_prop.properties.items() if sub_prop.properties else [])
            required_set.update(sub_prop.required or [])

    for key, value in unprocessed_props:
        prop_required = key in required_set
        prop_or_error: Property | (PropertyError | None)
        prop_or_error, schemas = property_from_data(
            name=key,
            required=prop_required,
            data=value,
            schemas=schemas,
            parent_name=class_name,
            config=config,
            roots=roots,
        )
        if not isinstance(prop_or_error, PropertyError):
            prop_or_error = _add_if_no_conflict(prop_or_error)
        if isinstance(prop_or_error, PropertyError):
            return prop_or_error

    required_properties = []
    optional_properties = []
    for prop in properties.values():
        if prop.required:
            required_properties.append(prop)
        else:
            optional_properties.append(prop)

        lazy_imports.update(prop.get_lazy_imports(prefix=".."))
        relative_imports.update(prop.get_imports(prefix=".."))

    return _PropertyData(
        optional_props=optional_properties,
        required_props=required_properties,
        relative_imports=relative_imports,
        lazy_imports=lazy_imports,
        schemas=schemas,
    )


ANY_ADDITIONAL_PROPERTY = AnyProperty.build(
    name="additional",
    required=True,
    default=None,
    description="",
    python_name=PythonIdentifier(value="additional", prefix=""),
    example=None,
)


def _get_additional_properties(
    *,
    schema_additional: None | (bool | (oai.Reference | oai.Schema)),
    schemas: Schemas,
    class_name: utils.ClassName,
    config: Config,
    roots: set[ReferencePath | utils.ClassName],
) -> tuple[Property | None | PropertyError, Schemas]:
    from . import property_from_data

    if schema_additional is None:
        return ANY_ADDITIONAL_PROPERTY, schemas

    if isinstance(schema_additional, bool):
        if schema_additional:
            return ANY_ADDITIONAL_PROPERTY, schemas
        return None, schemas

    if isinstance(schema_additional, oai.Schema) and not any(schema_additional.model_dump().values()):
        # An empty schema
        return ANY_ADDITIONAL_PROPERTY, schemas

    additional_properties, schemas = property_from_data(
        name="AdditionalProperty",
        required=True,  # in the sense that if present in the dict will not be None
        data=schema_additional,
        schemas=schemas,
        parent_name=class_name,
        config=config,
        roots=roots,
    )
    return additional_properties, schemas


def _process_property_data(
    *,
    data: oai.Schema,
    schemas: Schemas,
    class_info: Class,
    config: Config,
    roots: set[ReferencePath | utils.ClassName],
) -> tuple[tuple[_PropertyData, Property | None] | PropertyError, Schemas]:
    property_data = _process_properties(
        data=data, schemas=schemas, class_name=class_info.name, config=config, roots=roots
    )
    if isinstance(property_data, PropertyError):
        return property_data, schemas
    schemas = property_data.schemas

    additional_properties, schemas = _get_additional_properties(
        schema_additional=data.additionalProperties,
        schemas=schemas,
        class_name=class_info.name,
        config=config,
        roots=roots,
    )
    if isinstance(additional_properties, PropertyError):
        return additional_properties, schemas
    elif additional_properties is None:
        pass
    else:
        property_data.relative_imports.update(additional_properties.get_imports(prefix=".."))
        property_data.lazy_imports.update(additional_properties.get_lazy_imports(prefix=".."))

    return (property_data, additional_properties), schemas


def process_model(model_prop: ModelProperty, *, schemas: Schemas, config: Config) -> Schemas | PropertyError:
    """Populate a ModelProperty instance's property data
    Args:
        model_prop: The ModelProperty to build property data for
        schemas: Existing Schemas
        config: Config data for this run of the generator, used to modifying names
    Returns:
        Either the updated `schemas` input or a `PropertyError` if something went wrong.
    """
    data_or_err, schemas = _process_property_data(
        data=model_prop.data,
        schemas=schemas,
        class_info=model_prop.class_info,
        config=config,
        roots=model_prop.roots,
    )
    if isinstance(data_or_err, PropertyError):
        return data_or_err

    property_data, additional_properties = data_or_err

    object.__setattr__(model_prop, "required_properties", property_data.required_props)
    object.__setattr__(model_prop, "optional_properties", property_data.optional_props)
    model_prop.set_relative_imports(property_data.relative_imports)
    model_prop.set_lazy_imports(property_data.lazy_imports)
    object.__setattr__(model_prop, "additional_properties", additional_properties)
    return schemas
