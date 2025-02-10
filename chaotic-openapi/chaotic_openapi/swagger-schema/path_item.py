from pydantic import BaseModel
from typing import Optional, List, Dict, Any

class PathItem(BaseModel):
    summary: Optional[str] = None
    description: Optional[str] = None
    parameters: Optional[List[Dict[str, Any]]] = None
    responses: Dict[int, Any]
