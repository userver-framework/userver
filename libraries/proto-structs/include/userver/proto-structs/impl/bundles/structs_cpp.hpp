#pragma once

#include <userver/proto-structs/io/impl/field_accessor.hpp>
#include <userver/proto-structs/io/impl/read.hpp>
#include <userver/proto-structs/io/impl/write.hpp>
#include <userver/proto-structs/type_mapping.hpp>

#include <userver/proto-structs/io/std/optional_conv.hpp>
#include <userver/proto-structs/io/std/string_conv.hpp>
#include <userver/proto-structs/io/std/vector_conv.hpp>

#include <userver/proto-structs/io/std/int32_t_conv.hpp>
#include <userver/proto-structs/io/std/int64_t_conv.hpp>
#include <userver/proto-structs/io/std/size_t_conv.hpp>
#include <userver/proto-structs/io/std/uint32_t_conv.hpp>
#include <userver/proto-structs/io/std/uint64_t_conv.hpp>

// For keyword types.
#include <userver/proto-structs/io/std/scalar_conv.hpp>

#include <userver/proto-structs/io/userver/proto_structs/unbreakable_dependency_cycle_conv.hpp>
#include <userver/proto-structs/io/userver/utils/box_conv.hpp>
