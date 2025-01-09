from pydantic import BaseModel, ConfigDict, Field


class Reference(BaseModel):
    """
    A simple object to allow referencing other components in the specification, internally and externally.

    The Reference Object is defined by [JSON Reference](https://tools.ietf.org/html/draft-pbryan-zyp-json-ref-03)
    and follows the same structure, behavior and rules.

    For this specification, reference resolution is accomplished as defined by the JSON Reference specification
    and not by the JSON Schema specification.

    References:
        - https://swagger.io/docs/specification/using-ref/
        - https://github.com/OAI/OpenAPI-Specification/blob/main/versions/3.0.3.md#referenceObject
    """

    ref: str = Field(alias="$ref")
    model_config = ConfigDict(
        extra="allow",
        populate_by_name=True,
        json_schema_extra={
            "examples": [{"$ref": "#/components/schemas/Pet"}, {"$ref": "Pet.json"}, {"$ref": "definitions.json#/Pet"}]
        },
    )
