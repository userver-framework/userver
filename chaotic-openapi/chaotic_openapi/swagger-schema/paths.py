from pydantic import RootModel
from typing import Optional, List, Dict, Any
from path_item import PathItem

class Paths(RootModel):
    root: Dict[str, Dict[str, PathItem]]
