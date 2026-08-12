#pragma once

/// @file userver/proto-structs/io/supported_types_conv.hpp
/// @brief Bundle for all `proto-structs/io` *conv*-headers which provide read/write context class with the ability
///        to handle well-known types conversion.

#include <userver/proto-structs/io/std/chrono/duration_conv.hpp>
#include <userver/proto-structs/io/std/chrono/hh_mm_ss_conv.hpp>
#include <userver/proto-structs/io/std/chrono/time_point_conv.hpp>
#include <userver/proto-structs/io/std/chrono/year_month_day_conv.hpp>
#include <userver/proto-structs/io/std/map_conv.hpp>
#include <userver/proto-structs/io/std/optional_conv.hpp>
#include <userver/proto-structs/io/std/scalar_conv.hpp>
#include <userver/proto-structs/io/std/unordered_map_conv.hpp>
#include <userver/proto-structs/io/std/vector_conv.hpp>

#include <userver/proto-structs/io/userver/decimal64/decimal_conv.hpp>
#include <userver/proto-structs/io/userver/formats/json/array_conv.hpp>
#include <userver/proto-structs/io/userver/formats/json/object_conv.hpp>
#include <userver/proto-structs/io/userver/formats/json/value_conv.hpp>
#include <userver/proto-structs/io/userver/proto_structs/any_conv.hpp>
#include <userver/proto-structs/io/userver/proto_structs/date_conv.hpp>
#include <userver/proto-structs/io/userver/proto_structs/duration_conv.hpp>
#include <userver/proto-structs/io/userver/proto_structs/hash_map_conv.hpp>
#include <userver/proto-structs/io/userver/proto_structs/oneof_conv.hpp>
#include <userver/proto-structs/io/userver/proto_structs/time_of_day_conv.hpp>
#include <userver/proto-structs/io/userver/proto_structs/timestamp_conv.hpp>
#include <userver/proto-structs/io/userver/utils/box_conv.hpp>
#include <userver/proto-structs/io/userver/utils/datetime/time_of_day_conv.hpp>
#include <userver/proto-structs/io/userver/utils/strong_typedef_conv.hpp>
