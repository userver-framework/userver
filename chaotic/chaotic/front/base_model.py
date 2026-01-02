from typing import Any

import pydantic

IGNORE_PREFIXES = {
    'x-taxi-py3',
    'x-taxi-go',
    'x-taxi-driver',
}


class BaseModel(pydantic.BaseModel):
    model_config = pydantic.ConfigDict(extra='allow', strict=True)

    x_properties: dict[str, Any] = pydantic.Field(
        default_factory=dict,
        exclude=True,
    )

    @classmethod
    def model_userver_tags(cls) -> list[str]:
        return []

    def model_post_init(self, context: Any) -> None:
        super().model_post_init(context)

        if not self.__pydantic_extra__:
            return
        for field in self.__pydantic_extra__:
            ignore = False
            for prefix in IGNORE_PREFIXES:
                if field.startswith(prefix):
                    ignore = True
            if ignore:
                continue

            if field.startswith('x-taxi-') or field.startswith('x-usrv-'):
                assert field in self.model_userver_tags(), f'Field {field} is not allowed in this context'
                self.x_properties[field] = self.__pydantic_extra__[field]
                continue

            if field.startswith('x-'):
                self.x_properties[field] = self.__pydantic_extra__[field]
                continue

            known = [key for key in self.__pydantic_fields__ if not self.__pydantic_fields__[key].exclude]
            known.sort()
            raise Exception(f'Unknown field: "{field}", known fields: {known}')
