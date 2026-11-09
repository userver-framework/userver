import typing

import pydantic

from chaotic.front import base_model

BaseModel = base_model.BaseModel


class WithExpectedStatusCodes(BaseModel):
    x_expected_status_codes: list[typing.Annotated[int, pydantic.Field(strict=True, ge=100, le=599)]] | None = (
        pydantic.Field(
            default=None,
            validation_alias=pydantic.AliasChoices('x-taxi-expected-status-codes', 'x-usrv-expected-status-codes'),
        )
    )

    @pydantic.field_validator('x_expected_status_codes', mode='before')
    @classmethod
    def validate_expected_status_codes(cls, value):
        if not isinstance(value, list):
            raise ValueError('Expected HTTP status codes must be a list of integers from 100 to 599')
        return value


class XMiddlewares(pydantic.BaseModel):
    model_config = pydantic.ConfigDict(extra='allow')

    tvm: bool = True

    # TODO: validate field set
