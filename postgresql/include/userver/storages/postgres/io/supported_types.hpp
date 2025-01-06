#pragma once

/// @file userver/storages/postgres/io/supported_types.hpp
/// @brief Includes parsers and formatters for all supported data types;
/// prefer using the type specific include to avoid compilation time slowdown.

//@{
/** @name Traits etc */
#include <userver/storages/postgres/io/nullable_traits.hpp>
#include <userver/storages/postgres/io/traits.hpp>
#include <userver/storages/postgres/io/type_mapping.hpp>
#include <userver/storages/postgres/io/type_traits.hpp>
//@}

//@{
/** @name Data types */
#include <userver/storages/postgres/io/bytea.hpp>
#include <userver/storages/postgres/io/chrono.hpp>
#include <userver/storages/postgres/io/decimal64.hpp>
#include <userver/storages/postgres/io/enum_types.hpp>
#include <userver/storages/postgres/io/floating_point_types.hpp>
#include <userver/storages/postgres/io/integral_types.hpp>
#include <userver/storages/postgres/io/string_types.hpp>
#include <userver/storages/postgres/io/time_of_day.hpp>
#include <userver/storages/postgres/io/uuid.hpp>
//@}

//@{
/** @name Row types */
#include <userver/storages/postgres/io/row_types.hpp>
//@}

//@{
/** @name Type derivatives */
#include <userver/storages/postgres/io/array_types.hpp>
#include <userver/storages/postgres/io/composite_types.hpp>
#include <userver/storages/postgres/io/optional.hpp>
#include <userver/storages/postgres/io/range_types.hpp>
#include <userver/storages/postgres/io/strong_typedef.hpp>
//@}

//@{
/** @name JSON types */
#include <userver/storages/postgres/io/json_types.hpp>
//@}

//@{
/** @name User type registry */
#include <userver/storages/postgres/io/user_types.hpp>
//@}
