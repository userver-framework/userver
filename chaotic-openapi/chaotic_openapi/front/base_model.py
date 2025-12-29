import pydantic

from chaotic.front import base_model

BaseModel = base_model.BaseModel


class XMiddlewares(pydantic.BaseModel):
    model_config = pydantic.ConfigDict(extra='allow')

    tvm: bool = True

    # TODO: validate field set
