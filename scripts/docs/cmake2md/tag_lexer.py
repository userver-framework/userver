import dataclasses
import re


@dataclasses.dataclass
class Tag:
    name: str


STR_RE = re.compile('^([^@\s]+|^[\s]+)')
TAG_RE = re.compile('^@[a-z]*')


def tokenize(lines: list[str]) -> list[Tag | str]:
    result = []

    text = '\n'.join(lines)
    while text:
        if m := re.match(STR_RE, text):
            result.append(m.group())
            text = text[m.end() :]

        if m := re.match(TAG_RE, text):
            result.append(Tag(name=m.group()[1:]))
            text = text[m.end() :]

    return result
