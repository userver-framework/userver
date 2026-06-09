import pathlib
import pydantic

class CollectConfig(pydantic.BaseModel):
    from_sha: str
    to_sha: str
    repo_path: pathlib.Path = pydantic.Field(default_factory=pathlib.Path.cwd)