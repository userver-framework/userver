# Python 3.11 has StrEnum but breaks the old `str, Enum` hack.
# Unless this gets fixed, we need to have two implementations :(
import sys

if sys.version_info >= (3, 11):
    from enum import StrEnum

    class ParameterLocation(StrEnum):
        """The places Parameters can be put when calling an Endpoint"""

        QUERY = "query"
        PATH = "path"
        HEADER = "header"
        COOKIE = "cookie"

else:
    from enum import Enum

    class ParameterLocation(str, Enum):
        """The places Parameters can be put when calling an Endpoint"""

        QUERY = "query"
        PATH = "path"
        HEADER = "header"
        COOKIE = "cookie"
