import dataclasses
from enum import Enum

import tag_lexer


class ParamKind(str, Enum):
    Positional = 'arg'
    Option = 'option'
    SingleArgParam = 'param'
    MultiArgParam = 'multiparam'


@dataclasses.dataclass
class Param:
    kind: ParamKind
    name: str
    description: str
    required: bool


@dataclasses.dataclass
class Prototype:
    description: str
    args: list[Param]
    options: list[Param]
    params: list[Param]
    multi_params: list[Param]


class Parser:
    def __init__(self) -> None:
        self.proto_description = ''
        self.params = list[Param]()

        self.kind: ParamKind | None = None
        self.description = ''
        self.required = False

    def _finalize_param(self) -> None:
        if self.kind:
            self.params.append(
                Param(
                    kind=self.kind,
                    name=self.name,
                    description=self.description.strip(),
                    required=self.required,
                )
            )
        else:
            if self.description:
                self.proto_description = self.description.strip()

        self.kind = None
        self.name = ''
        self.description = ''
        self.required = False

    def parse(self, tokens: list[tag_lexer.Tag | str]) -> Prototype:
        it = iter(tokens)
        for token in it:
            if isinstance(token, str):
                self.description += token
                continue

            tag = token
            assert isinstance(tag, tag_lexer.Tag)
            match tag.name:
                case ParamKind.Option | ParamKind.Positional | ParamKind.SingleArgParam | ParamKind.MultiArgParam:
                    self._finalize_param()
                    self.kind = ParamKind(tag.name)
                    self.name = next(it).strip()
                    if not self.name:
                        # skip spaces, read next non-space token
                        self.name = next(it).strip()
                    self.required = tag.name == ParamKind.Positional

                case 'required':
                    self.required = True

                case _:
                    raise BaseException(f'Unknown tag: @{tag.name}')
        self._finalize_param()

        def filter_kind(kind: ParamKind, params: list[Param]):
            def match(x: Param) -> bool:
                return x.kind == kind

            return list(filter(match, params))

        return Prototype(
            description=self.proto_description,
            options=filter_kind(ParamKind.Option, self.params),
            args=filter_kind(ParamKind.Positional, self.params),
            params=filter_kind(ParamKind.SingleArgParam, self.params),
            multi_params=filter_kind(ParamKind.MultiArgParam, self.params),
        )


def parse(tokens: list[tag_lexer.Tag | str]) -> Prototype:
    return Parser().parse(tokens)
