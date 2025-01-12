Everything in this directory (including the rest of this file after this paragraph) is a vendored copy of [openapi-schem-pydantic](https://github.com/kuimono/openapi-schema-pydantic) and is licensed under the LICENSE file in this directory.

Included vendored version is the [following](https://github.com/kuimono/openapi-schema-pydantic/commit/0836b429086917feeb973de3367a7ac4c2b3a665)
Small patches has been applied to it.

## Alias

Due to the reserved words in python and pydantic,
the following fields are used with [alias](https://pydantic-docs.helpmanual.io/usage/schema/#field-customisation) feature provided by pydantic:

| Class | Field name in the class | Alias (as in OpenAPI spec) |
| ----- | ----------------------- | -------------------------- |
| Header[*](#header_param_in) | param_in | in |
| MediaType | media_type_schema | schema |
| Parameter | param_in | in |
| Parameter | param_schema | schema |
| PathItem | ref | $ref |
| Reference | ref | $ref |
| SecurityScheme | security_scheme_in | in |
| Schema | schema_format | format |
| Schema | schema_not | not |

> <a name="header_param_in"></a>The "in" field in Header object is actually a constant (`{"in": "header"}`).

> For convenience of object creation, the classes mentioned in above
> has configured `allow_population_by_field_name=True`.
>
> Reference: [Pydantic's Model Config](https://pydantic-docs.helpmanual.io/usage/model_config/)

## Non-pydantic schema types

Due to the constriants of python typing structure (not able to handle dynamic field names),
the following schema classes are actually just a typing of `Dict`:

| Schema Type | Implementation |
| ----------- | -------------- |
| Callback | `Callback = Dict[str, PathItem]` |
| Paths | `Paths = Dict[str, PathItem]` |
| Responses | `Responses = Dict[str, Union[Response, Reference]]` |
| SecurityRequirement | `SecurityRequirement = Dict[str, List[str]]` |

On creating such schema instances, please use python's `dict` type instead to instantiate.
