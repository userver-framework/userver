#pragma once

/// @file userver/proto-structs/io/supported_types.hpp
/// @brief Bundle for all `proto-structs/io` headers which provide well-known types support to proto structs.

#include <userver/proto-structs/io/std/chrono/duration.hpp>
#include <userver/proto-structs/io/std/chrono/hh_mm_ss.hpp>
#include <userver/proto-structs/io/std/chrono/time_point.hpp>
#include <userver/proto-structs/io/std/chrono/year_month_day.hpp>
#include <userver/proto-structs/io/std/map.hpp>
#include <userver/proto-structs/io/std/optional.hpp>
#include <userver/proto-structs/io/std/scalar.hpp>
#include <userver/proto-structs/io/std/unordered_map.hpp>
#include <userver/proto-structs/io/std/vector.hpp>

#include <userver/proto-structs/io/userver/decimal64/decimal.hpp>
#include <userver/proto-structs/io/userver/formats/json/array.hpp>
#include <userver/proto-structs/io/userver/formats/json/object.hpp>
#include <userver/proto-structs/io/userver/formats/json/value.hpp>
#include <userver/proto-structs/io/userver/proto_structs/any.hpp>
#include <userver/proto-structs/io/userver/proto_structs/date.hpp>
#include <userver/proto-structs/io/userver/proto_structs/duration.hpp>
#include <userver/proto-structs/io/userver/proto_structs/hash_map.hpp>
#include <userver/proto-structs/io/userver/proto_structs/oneof.hpp>
#include <userver/proto-structs/io/userver/proto_structs/time_of_day.hpp>
#include <userver/proto-structs/io/userver/proto_structs/timestamp.hpp>
#include <userver/proto-structs/io/userver/utils/box.hpp>
#include <userver/proto-structs/io/userver/utils/datetime/time_of_day.hpp>
#include <userver/proto-structs/io/userver/utils/strong_typedef.hpp>
