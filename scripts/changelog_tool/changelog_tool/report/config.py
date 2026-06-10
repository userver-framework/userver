import pathlib
import pydantic

class ReportConfig(pydantic.BaseModel):
    github_url: str
    output_dir: pathlib.Path = pydantic.Field(default_factory=lambda: pathlib.Path(".changelog"))
