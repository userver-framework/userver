import sys  # noqa: I001

from chaotic.front import types
from chaotic_openapi.front import model


# TODO: way to ignore error
def report_error(errtype: str, infile_path: str, msg: str) -> None:
    print(f'{infile_path.filepath}#{infile_path.location}: [{errtype}]: {msg}', file=sys.stderr)


def validate(service: model.Service) -> None:
    validate_nonobject_body(service)
    validate_dash_in_field_name(service)
    validate_free_parameter_in_path(service)


def validate_nonobject_body(service: model.Service) -> None:
    for operation in service.operations:
        for content_type, body in operation.requestBody.items():
            if isinstance(body, types.Ref):
                schema = service.schemas[body.ref]
            else:
                schema = body

            if not isinstance(
                schema, (types.SchemaObject, types.OneOfWithDiscriminator, types.OneOfWithoutDiscriminator)
            ):
                print(schema, file=sys.stderr)
                report_error('non-object-body', schema.source_location(), 'Non-object type in body root is forbidden')


def validate_free_parameter_in_path(service: model.Service) -> None:
    pass


def validate_dash_in_field_name(service: model.Service) -> None:
    def visitor(child: types.Schema, _: types.Schema):
        if isinstance(child, types.SchemaObject):
            for property in child.properties:
                if '-' not in property:
                    continue

                report_error(
                    'dash-in-field-name', child.source_location(), 'Dash in field name is useless for JS/Typescript'
                )

    for operation in service.operations:
        for content_type, body in operation.requestBody.items():
            if isinstance(body, types.Ref):
                schema = service.schemas[body.ref]
            else:
                schema = body

            visitor(schema, schema)
            schema.visit_children(visitor)
