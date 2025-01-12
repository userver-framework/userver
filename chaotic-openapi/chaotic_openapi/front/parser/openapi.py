import re
from collections.abc import Iterator
from copy import deepcopy
from dataclasses import dataclass, field
from http import HTTPStatus
from typing import Any, Optional, Protocol, Union

from pydantic import ValidationError

from .. import schema as oai
from .. import utils
from ..config import Config
from ..utils import PythonIdentifier
from .bodies import Body, body_from_data
from .errors import GeneratorError, ParseError, PropertyError
from .properties import (
    Class,
    EnumProperty,
    LiteralEnumProperty,
    ModelProperty,
    Parameters,
    Property,
    Schemas,
    build_parameters,
    build_schemas,
    property_from_data,
)
from .properties.schemas import parameter_from_reference
from .responses import Response, response_from_data

_PATH_PARAM_REGEX = re.compile("{([a-zA-Z_-][a-zA-Z0-9_-]*)}")


def import_string_from_class(class_: Class, prefix: str = "") -> str:
    """Create a string which is used to import a reference"""
    return f"from {prefix}.{class_.module_name} import {class_.name}"


@dataclass
class EndpointCollection:
    """A bunch of endpoints grouped under a tag that will become a module"""

    tag: str
    endpoints: list["Endpoint"] = field(default_factory=list)
    parse_errors: list[ParseError] = field(default_factory=list)

    @staticmethod
    def from_data(
        *,
        data: dict[str, oai.PathItem],
        schemas: Schemas,
        parameters: Parameters,
        request_bodies: dict[str, Union[oai.RequestBody, oai.Reference]],
        config: Config,
    ) -> tuple[dict[utils.PythonIdentifier, "EndpointCollection"], Schemas, Parameters]:
        """Parse the openapi paths data to get EndpointCollections by tag"""
        endpoints_by_tag: dict[utils.PythonIdentifier, EndpointCollection] = {}

        methods = ["get", "put", "post", "delete", "options", "head", "patch", "trace"]

        for path, path_data in data.items():
            for method in methods:
                operation: Optional[oai.Operation] = getattr(path_data, method)
                if operation is None:
                    continue

                tags = [utils.PythonIdentifier(value=tag, prefix="tag") for tag in operation.tags or ["default"]]
                if not config.generate_all_tags:
                    tags = tags[:1]

                collections = [endpoints_by_tag.setdefault(tag, EndpointCollection(tag=tag)) for tag in tags]

                endpoint, schemas, parameters = Endpoint.from_data(
                    data=operation,
                    path=path,
                    method=method,
                    tags=tags,
                    schemas=schemas,
                    parameters=parameters,
                    request_bodies=request_bodies,
                    config=config,
                )
                # Add `PathItem` parameters
                if not isinstance(endpoint, ParseError):
                    endpoint, schemas, parameters = Endpoint.add_parameters(
                        endpoint=endpoint,
                        data=path_data,
                        schemas=schemas,
                        parameters=parameters,
                        config=config,
                    )
                if not isinstance(endpoint, ParseError):
                    endpoint = Endpoint.sort_parameters(endpoint=endpoint)
                if isinstance(endpoint, ParseError):
                    endpoint.header = f"WARNING parsing {method.upper()} {path} within {'/'.join(tags)}. Endpoint will not be generated."
                    for collection in collections:
                        collection.parse_errors.append(endpoint)
                    continue
                for error in endpoint.errors:
                    error.header = f"WARNING parsing {method.upper()} {path} within {'/'.join(tags)}."
                    for collection in collections:
                        collection.parse_errors.append(error)
                for collection in collections:
                    collection.endpoints.append(endpoint)

        return endpoints_by_tag, schemas, parameters


def generate_operation_id(*, path: str, method: str) -> str:
    """Generate an operationId from a path"""
    clean_path = path.replace("{", "").replace("}", "").replace("/", "_")
    if clean_path.startswith("_"):
        clean_path = clean_path[1:]
    if clean_path.endswith("_"):
        clean_path = clean_path[:-1]
    return f"{method}_{clean_path}"


models_relative_prefix: str = "..."


class RequestBodyParser(Protocol):
    __name__: str = "RequestBodyParser"

    def __call__(
        self, *, body: oai.RequestBody, schemas: Schemas, parent_name: str, config: Config
    ) -> tuple[Union[Property, PropertyError, None], Schemas]: ...  # pragma: no cover


@dataclass
class Endpoint:
    """
    Describes a single endpoint on the server
    """

    path: str
    method: str
    description: Optional[str]
    name: str
    requires_security: bool
    tags: list[PythonIdentifier]
    summary: Optional[str] = ""
    relative_imports: set[str] = field(default_factory=set)
    query_parameters: list[Property] = field(default_factory=list)
    path_parameters: list[Property] = field(default_factory=list)
    header_parameters: list[Property] = field(default_factory=list)
    cookie_parameters: list[Property] = field(default_factory=list)
    responses: list[Response] = field(default_factory=list)
    bodies: list[Body] = field(default_factory=list)
    errors: list[ParseError] = field(default_factory=list)

    @staticmethod
    def _add_responses(
        *, endpoint: "Endpoint", data: oai.Responses, schemas: Schemas, config: Config
    ) -> tuple["Endpoint", Schemas]:
        endpoint = deepcopy(endpoint)
        for code, response_data in data.items():
            status_code: HTTPStatus
            try:
                status_code = HTTPStatus(int(code))
            except ValueError:
                endpoint.errors.append(
                    ParseError(
                        detail=(
                            f"Invalid response status code {code} (not a valid HTTP "
                            f"status code), response will be omitted from generated "
                            f"client"
                        )
                    )
                )
                continue

            response, schemas = response_from_data(
                status_code=status_code,
                data=response_data,
                schemas=schemas,
                parent_name=endpoint.name,
                config=config,
            )
            if isinstance(response, ParseError):
                detail_suffix = "" if response.detail is None else f" ({response.detail})"
                endpoint.errors.append(
                    ParseError(
                        detail=(
                            f"Cannot parse response for status code {status_code}{detail_suffix}, "
                            f"response will be omitted from generated client"
                        ),
                        data=response.data,
                    )
                )
                continue

            # No reasons to use lazy imports in endpoints, so add lazy imports to relative here.
            endpoint.relative_imports |= response.prop.get_lazy_imports(prefix=models_relative_prefix)
            endpoint.relative_imports |= response.prop.get_imports(prefix=models_relative_prefix)
            endpoint.responses.append(response)
        return endpoint, schemas

    @staticmethod
    def add_parameters(
        *,
        endpoint: "Endpoint",
        data: Union[oai.Operation, oai.PathItem],
        schemas: Schemas,
        parameters: Parameters,
        config: Config,
    ) -> tuple[Union["Endpoint", ParseError], Schemas, Parameters]:
        """Process the defined `parameters` for an Endpoint.

        Any existing parameters will be ignored, so earlier instances of a parameter take precedence. PathItem
        parameters should therefore be added __after__ operation parameters.

        Args:
            endpoint: The endpoint to add parameters to.
            data: The Operation or PathItem to add parameters from.
            schemas: The cumulative Schemas of processing so far which should contain details for any references.
            parameters: The cumulative Parameters of processing so far which should contain details for any references.
            config: User-provided config for overrides within parameters.

        Returns:
            `(result, schemas, parameters)` where `result` is either an updated Endpoint containing the parameters or a
            ParseError describing what went wrong. `schemas` is an updated version of the `schemas` input, adding any
            new enums or classes. `parameters` is an updated version of the `parameters` input, adding new parameters.

        See Also:
            - https://swagger.io/docs/specification/describing-parameters/
            - https://swagger.io/docs/specification/paths-and-operations/
        """
        # There isn't much value in breaking down this function further other than to satisfy the linter.

        if data.parameters is None:
            return endpoint, schemas, parameters

        endpoint = deepcopy(endpoint)

        unique_parameters: set[tuple[str, oai.ParameterLocation]] = set()
        parameters_by_location: dict[str, list[Property]] = {
            oai.ParameterLocation.QUERY: endpoint.query_parameters,
            oai.ParameterLocation.PATH: endpoint.path_parameters,
            oai.ParameterLocation.HEADER: endpoint.header_parameters,
            oai.ParameterLocation.COOKIE: endpoint.cookie_parameters,
        }

        for param in data.parameters:
            # Obtain the parameter from the reference or just the parameter itself
            param_or_error = parameter_from_reference(param=param, parameters=parameters)
            if isinstance(param_or_error, ParseError):
                return param_or_error, schemas, parameters
            param = param_or_error  # noqa: PLW2901

            if param.param_schema is None:
                continue

            unique_param = (param.name, param.param_in)
            if unique_param in unique_parameters:
                return (
                    ParseError(
                        data=data,
                        detail=(
                            "Parameters MUST NOT contain duplicates. "
                            "A unique parameter is defined by a combination of a name and location. "
                            f"Duplicated parameters named `{param.name}` detected in `{param.param_in}`."
                        ),
                    ),
                    schemas,
                    parameters,
                )

            unique_parameters.add(unique_param)

            if any(
                other_param for other_param in parameters_by_location[param.param_in] if other_param.name == param.name
            ):
                # Defined at the operation level, ignore it here
                continue

            prop, new_schemas = property_from_data(
                name=param.name,
                required=param.required,
                data=param.param_schema,
                schemas=schemas,
                parent_name=endpoint.name,
                config=config,
            )

            if isinstance(prop, ParseError):
                return (
                    ParseError(
                        detail=f"cannot parse parameter of endpoint {endpoint.name}: {prop.detail}",
                        data=prop.data,
                    ),
                    schemas,
                    parameters,
                )

            schemas = new_schemas

            location_error = prop.validate_location(param.param_in)
            if location_error is not None:
                location_error.data = param
                return location_error, schemas, parameters

            # No reasons to use lazy imports in endpoints, so add lazy imports to relative here.
            endpoint.relative_imports.update(prop.get_lazy_imports(prefix=models_relative_prefix))
            endpoint.relative_imports.update(prop.get_imports(prefix=models_relative_prefix))
            parameters_by_location[param.param_in].append(prop)

        return endpoint._check_parameters_for_conflicts(config=config), schemas, parameters

    def _check_parameters_for_conflicts(
        self,
        *,
        config: Config,
        previously_modified_params: Optional[set[tuple[oai.ParameterLocation, str]]] = None,
    ) -> Union["Endpoint", ParseError]:
        """Check for conflicting parameters

        For parameters that have the same python_name but are in different locations, append the location to the
        python_name. For parameters that have the same name but are in the same location, use their raw name without
        snake casing instead.

        Function stops when there's a conflict that can't be resolved or all parameters are guaranteed to have a
        unique python_name.
        """
        modified_params = previously_modified_params or set()
        used_python_names: dict[PythonIdentifier, tuple[oai.ParameterLocation, Property]] = {}
        reserved_names = ["client", "url"]
        for parameter in self.iter_all_parameters():
            location, prop = parameter

            if prop.python_name in reserved_names:
                prop.set_python_name(new_name=f"{prop.python_name}_{location}", config=config)
                modified_params.add((location, prop.name))
                continue

            conflicting = used_python_names.pop(prop.python_name, None)
            if conflicting is None:
                used_python_names[prop.python_name] = parameter
                continue
            conflicting_location, conflicting_prop = conflicting
            if (conflicting_location, conflicting_prop.name) in modified_params or (
                location,
                prop.name,
            ) in modified_params:
                return ParseError(
                    detail=f"Parameters with same Python identifier {conflicting_prop.python_name} detected",
                )

            if location != conflicting_location:
                conflicting_prop.set_python_name(
                    new_name=f"{conflicting_prop.python_name}_{conflicting_location}", config=config
                )
                prop.set_python_name(new_name=f"{prop.python_name}_{location}", config=config)
            elif conflicting_prop.name != prop.name:  # Use the name to differentiate
                conflicting_prop.set_python_name(new_name=conflicting_prop.name, config=config, skip_snake_case=True)
                prop.set_python_name(new_name=prop.name, config=config, skip_snake_case=True)

            modified_params.add((location, conflicting_prop.name))
            modified_params.add((conflicting_location, conflicting_prop.name))
            used_python_names[prop.python_name] = parameter
            used_python_names[conflicting_prop.python_name] = conflicting

        if len(modified_params) > 0 and modified_params != previously_modified_params:
            return self._check_parameters_for_conflicts(config=config, previously_modified_params=modified_params)
        return self

    @staticmethod
    def sort_parameters(*, endpoint: "Endpoint") -> Union["Endpoint", ParseError]:
        """
        Sorts the path parameters of an `endpoint` so that they match the order declared in `endpoint.path`.

        Args:
            endpoint: The endpoint to sort the parameters of.

        Returns:
            Either an updated `endpoint` with sorted path parameters or a `ParseError` if something was wrong with
                the path parameters and they could not be sorted.
        """
        endpoint = deepcopy(endpoint)
        parameters_from_path = re.findall(_PATH_PARAM_REGEX, endpoint.path)
        try:
            endpoint.path_parameters.sort(
                key=lambda param: parameters_from_path.index(param.name),
            )
        except ValueError:
            pass  # We're going to catch the difference down below

        if parameters_from_path != [param.name for param in endpoint.path_parameters]:
            return ParseError(
                detail=f"Incorrect path templating for {endpoint.path} (Path parameters do not match with path)",
            )
        for parameter in endpoint.path_parameters:
            endpoint.path = endpoint.path.replace(f"{{{parameter.name}}}", f"{{{parameter.python_name}}}")
        return endpoint

    @staticmethod
    def from_data(
        *,
        data: oai.Operation,
        path: str,
        method: str,
        tags: list[PythonIdentifier],
        schemas: Schemas,
        parameters: Parameters,
        request_bodies: dict[str, Union[oai.RequestBody, oai.Reference]],
        config: Config,
    ) -> tuple[Union["Endpoint", ParseError], Schemas, Parameters]:
        """Construct an endpoint from the OpenAPI data"""

        if data.operationId is None:
            name = generate_operation_id(path=path, method=method)
        else:
            name = data.operationId

        endpoint = Endpoint(
            path=path,
            method=method,
            summary=utils.remove_string_escapes(data.summary) if data.summary else "",
            description=utils.remove_string_escapes(data.description) if data.description else "",
            name=name,
            requires_security=bool(data.security),
            tags=tags,
        )

        result, schemas, parameters = Endpoint.add_parameters(
            endpoint=endpoint,
            data=data,
            schemas=schemas,
            parameters=parameters,
            config=config,
        )
        if isinstance(result, ParseError):
            return result, schemas, parameters
        result, schemas = Endpoint._add_responses(endpoint=result, data=data.responses, schemas=schemas, config=config)
        if isinstance(result, ParseError):
            return result, schemas, parameters
        bodies, schemas = body_from_data(
            data=data, schemas=schemas, config=config, endpoint_name=result.name, request_bodies=request_bodies
        )
        body_errors = []
        for body in bodies:
            if isinstance(body, ParseError):
                body_errors.append(body)
                continue
            result.bodies.append(body)
            result.relative_imports.update(body.prop.get_imports(prefix=models_relative_prefix))
            result.relative_imports.update(body.prop.get_lazy_imports(prefix=models_relative_prefix))
        if len(result.bodies) > 0:
            result.errors.extend(body_errors)
        elif len(body_errors) > 0:
            return (
                ParseError(
                    header="Endpoint requires a body, but none were parseable.",
                    detail="\n".join(error.detail or "" for error in body_errors),
                ),
                schemas,
                parameters,
            )

        return result, schemas, parameters

    def response_type(self) -> str:
        """Get the Python type of any response from this endpoint"""
        types = sorted({response.prop.get_type_string(quoted=False) for response in self.responses})
        if len(types) == 0:
            return "Any"
        if len(types) == 1:
            return self.responses[0].prop.get_type_string(quoted=False)
        return f"Union[{', '.join(types)}]"

    def iter_all_parameters(self) -> Iterator[tuple[oai.ParameterLocation, Property]]:
        """Iterate through all the parameters of this endpoint"""
        yield from ((oai.ParameterLocation.PATH, param) for param in self.path_parameters)
        yield from ((oai.ParameterLocation.QUERY, param) for param in self.query_parameters)
        yield from ((oai.ParameterLocation.HEADER, param) for param in self.header_parameters)
        yield from ((oai.ParameterLocation.COOKIE, param) for param in self.cookie_parameters)

    def list_all_parameters(self) -> list[Property]:
        """Return a list of all the parameters of this endpoint"""
        return (
            self.path_parameters
            + self.query_parameters
            + self.header_parameters
            + self.cookie_parameters
            + [body.prop for body in self.bodies]
        )


@dataclass
class GeneratorData:
    """All the data needed to generate a client"""

    title: str
    description: Optional[str]
    version: str
    models: Iterator[ModelProperty]
    errors: list[ParseError]
    endpoint_collections_by_tag: dict[utils.PythonIdentifier, EndpointCollection]
    enums: Iterator[Union[EnumProperty, LiteralEnumProperty]]

    @staticmethod
    def from_dict(data: dict[str, Any], *, config: Config) -> Union["GeneratorData", GeneratorError]:
        """Create an OpenAPI from dict"""
        try:
            openapi = oai.OpenAPI.model_validate(data)
        except ValidationError as err:
            detail = str(err)
            if "swagger" in data:
                detail = (
                    "You may be trying to use a Swagger document; this is not supported by this project.\n\n" + detail
                )
            return GeneratorError(header="Failed to parse OpenAPI document", detail=detail)
        schemas = Schemas()
        parameters = Parameters()
        if openapi.components and openapi.components.schemas:
            schemas = build_schemas(components=openapi.components.schemas, schemas=schemas, config=config)
        if openapi.components and openapi.components.parameters:
            parameters = build_parameters(
                components=openapi.components.parameters,
                parameters=parameters,
                config=config,
            )
        request_bodies = (openapi.components and openapi.components.requestBodies) or {}
        endpoint_collections_by_tag, schemas, parameters = EndpointCollection.from_data(
            data=openapi.paths, schemas=schemas, parameters=parameters, request_bodies=request_bodies, config=config
        )

        enums = (
            prop for prop in schemas.classes_by_name.values() if isinstance(prop, (EnumProperty, LiteralEnumProperty))
        )
        models = (prop for prop in schemas.classes_by_name.values() if isinstance(prop, ModelProperty))

        return GeneratorData(
            title=openapi.info.title,
            description=openapi.info.description,
            version=openapi.info.version,
            endpoint_collections_by_tag=endpoint_collections_by_tag,
            models=models,
            errors=schemas.errors + parameters.errors,
            enums=enums,
        )
