"""Sets of includes that are in bundles."""

from typing import Set

BUNDLE_HPP: Set[str] = {
    'cstdint',
    'limits',
    'optional',
    'string',
    'utility',
    'vector',
    'userver/proto-structs/impl/oneof_codegen.hpp',
    'userver/proto-structs/io/fwd.hpp',
    'userver/proto-structs/io/std/optional.hpp',
    'userver/proto-structs/io/std/string.hpp',
    'userver/proto-structs/io/std/vector.hpp',
    'userver/proto-structs/io/std/int32_t.hpp',
    'userver/proto-structs/io/std/int64_t.hpp',
    'userver/proto-structs/io/std/size_t.hpp',
    'userver/proto-structs/io/std/uint32_t.hpp',
    'userver/proto-structs/io/std/uint64_t.hpp',
    'userver/proto-structs/io/std/scalar_conv.hpp',
}

BUNDLE_CPP: Set[str] = {
    'userver/proto-structs/io/impl/field_accessor.hpp',
    'userver/proto-structs/io/impl/read.hpp',
    'userver/proto-structs/io/impl/write.hpp',
    'userver/proto-structs/type_mapping.hpp',
    'userver/proto-structs/io/std/optional_conv.hpp',
    'userver/proto-structs/io/std/string_conv.hpp',
    'userver/proto-structs/io/std/vector_conv.hpp',
    'userver/proto-structs/io/std/int32_t_conv.hpp',
    'userver/proto-structs/io/std/int64_t_conv.hpp',
    'userver/proto-structs/io/std/size_t_conv.hpp',
    'userver/proto-structs/io/std/uint32_t_conv.hpp',
    'userver/proto-structs/io/std/uint64_t_conv.hpp',
    'userver/proto-structs/io/std/scalar_conv.hpp',
}
