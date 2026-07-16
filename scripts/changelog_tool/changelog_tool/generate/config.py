"""Configuration model for the generate command."""

import pathlib

import pydantic


class GenerateConfig(pydantic.BaseModel):
    from_sha: str
    to_sha: str
    repo_path: pathlib.Path = pydantic.Field(default_factory=pathlib.Path.cwd)
    core_team_patterns: list[str] = pydantic.Field(default_factory=list)
    output_dir: pathlib.Path = pydantic.Field(default_factory=lambda: pathlib.Path('.changelog'))

    @pydantic.field_validator('repo_path', mode='after')
    @classmethod
    def _expand_repo_path(cls, value: pathlib.Path) -> pathlib.Path:
        # Without expanduser, a tilde-prefixed config value is kept as a
        # literal path. When used as subprocess cwd, a nonexistent directory
        # raises FileNotFoundError blaming the executable.
        return value.expanduser().resolve()
