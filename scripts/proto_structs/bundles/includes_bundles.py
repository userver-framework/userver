"""Sets of includes that are in C++ bundles."""

from collections.abc import Set
import functools
import re

from proto_structs.bundles import resources

_INCLUDE_CONTENTS_REGEX = re.compile(r'#include <([^>]+)>')


def _extract_includes(path_relative_to_userver: str, *, sanity_check_has_include: str) -> Set[str]:
    file_contents = resources.read_file_text(path_relative_to_userver)
    includes = {match.group(1) for match in _INCLUDE_CONTENTS_REGEX.finditer(file_contents)}
    assert sanity_check_has_include in includes, f'{path_relative_to_userver} must contain {sanity_check_has_include}'
    return includes


@functools.cache
def bundle_hpp() -> Set[str]:
    return _extract_includes(
        'libraries/proto-structs/include/userver/proto-structs/impl/bundles/structs_hpp.hpp',
        sanity_check_has_include='userver/proto-structs/io/std/string.hpp',
    )


@functools.cache
def bundle_cpp() -> Set[str]:
    return _extract_includes(
        'libraries/proto-structs/include/userver/proto-structs/impl/bundles/structs_cpp.hpp',
        sanity_check_has_include='userver/proto-structs/io/std/string_conv.hpp',
    )
