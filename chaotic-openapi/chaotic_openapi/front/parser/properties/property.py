__all__ = ["Property"]

from typing import Union

from typing_extensions import TypeAlias

from .any import AnyProperty
from .boolean import BooleanProperty
from .const import ConstProperty
from .date import DateProperty
from .datetime import DateTimeProperty
from .enum_property import EnumProperty
from .file import FileProperty
from .float import FloatProperty
from .int import IntProperty
from .list_property import ListProperty
from .literal_enum_property import LiteralEnumProperty
from .model_property import ModelProperty
from .none import NoneProperty
from .string import StringProperty
from .union import UnionProperty
from .uuid import UuidProperty

Property: TypeAlias = Union[
    AnyProperty,
    BooleanProperty,
    ConstProperty,
    DateProperty,
    DateTimeProperty,
    EnumProperty,
    LiteralEnumProperty,
    FileProperty,
    FloatProperty,
    IntProperty,
    ListProperty,
    ModelProperty,
    NoneProperty,
    StringProperty,
    UnionProperty,
    UuidProperty,
]
