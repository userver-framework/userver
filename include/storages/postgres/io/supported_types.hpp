#pragma once

//@{
/** @name Traits etc */
#include <storages/postgres/io/nullable_traits.hpp>
#include <storages/postgres/io/traits.hpp>
#include <storages/postgres/io/type_mapping.hpp>
#include <storages/postgres/io/type_traits.hpp>
//@}

//@{
/** @name Data types */
#include <storages/postgres/io/boost_multiprecision.hpp>
#include <storages/postgres/io/chrono.hpp>
#include <storages/postgres/io/floating_point_types.hpp>
#include <storages/postgres/io/integral_types.hpp>
#include <storages/postgres/io/string_types.hpp>
//@}

//@{
/** @name Type derivatives */
#include <storages/postgres/io/array_types.hpp>
#include <storages/postgres/io/composite_types.hpp>
#include <storages/postgres/io/optional.hpp>
//@}

//@{
/** @name Stream text IO */
#include <storages/postgres/io/stream_text_formatter.hpp>
#include <storages/postgres/io/stream_text_parser.hpp>
//@}

//@{
/** @name User type registry */
#include <storages/postgres/io/user_types.hpp>
//@}
