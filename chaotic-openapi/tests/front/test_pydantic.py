from chaotic_openapi.front import errors
import pydantic
import pytest

import chaotic.error


class Model(pydantic.BaseModel):
    model_config = pydantic.ConfigDict(extra='forbid', strict=True)

    field: str

    def model_post_init(self, context):
        if self.field == 'wrong':
            raise ValueError('wrong field value')


def wrap_error(model) -> None:
    try:
        Model(**model)
    except pydantic.ValidationError as exc:
        raise errors.convert_error(full_filepath='filepath', schema_type='jsonschema', err=exc) from None


def test_ok():
    model = {'field': 'foo'}
    wrap_error(model)


def test_extra_field():
    model = {'field': 'foo', 'extra': 'bar'}
    with pytest.raises(chaotic.error.BaseError) as exc:
        wrap_error(model)
    assert (
        str(exc.value)
        == """
===============================================================
Unhandled error while processing filepath
Path "extra", Format "jsonschema"
Error:
Extra fields are forbidden (bar)
==============================================================="""
    )


def test_missing_field():
    model = {}
    with pytest.raises(chaotic.error.BaseError) as exc:
        wrap_error(model)
    assert (
        str(exc.value)
        == """
===============================================================
Unhandled error while processing filepath
Path "field", Format "jsonschema"
Error:
Required field "field" is missing
==============================================================="""
    )


def test_wrong_field_type():
    model = {'field': 1}
    with pytest.raises(chaotic.error.BaseError) as exc:
        wrap_error(model)
    assert (
        str(exc.value)
        == """
===============================================================
Unhandled error while processing filepath
Path "field", Format "jsonschema"
Error:
String type is expected, 1 is found
==============================================================="""
    )


def test_model_post_init():
    model = {'field': 'wrong'}
    with pytest.raises(chaotic.error.BaseError) as exc:
        wrap_error(model)
    assert (
        str(exc.value)
        == """
===============================================================
Unhandled error while processing filepath
Path "", Format "jsonschema"
Error:
Value error, wrong field value
==============================================================="""
    )
