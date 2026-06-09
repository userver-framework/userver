import pathlib
import pydantic
from typing import List

class CollectConfig(pydantic.BaseModel):
    from_sha: str
    to_sha: str
    repo_path: pathlib.Path = pydantic.Field(default_factory=pathlib.Path.cwd)
    core_team_patterns: List[str] = pydantic.Field(default_factory=list)
    output_dir: pathlib.Path = pydantic.Field(default_factory=lambda: pathlib.Path(".changelog"))