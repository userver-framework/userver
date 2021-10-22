#include <gtest/gtest.h>

#include <cstdint>

#include <userver/storages/postgres/io/integral_types.hpp>
#include <userver/storages/postgres/io/pg_types.hpp>
#include <userver/storages/postgres/io/type_mapping.hpp>

USERVER_NAMESPACE_BEGIN

namespace io = storages::postgres::io;

TEST(PostgreIOInternal, SameTypeMapping) {
  EXPECT_TRUE(
      io::MappedToSameType(io::PredefinedOids::kOid, io::PredefinedOids::kTid));
  EXPECT_TRUE(
      io::MappedToSameType(io::PredefinedOids::kTid, io::PredefinedOids::kOid));
  EXPECT_FALSE(io::MappedToSameType(io::PredefinedOids::kTid,
                                    io::PredefinedOids::kInt8));

  EXPECT_TRUE(io::MappedToSameType(io::PredefinedOids::kOidArray,
                                   io::PredefinedOids::kTidArray));
  EXPECT_TRUE(io::MappedToSameType(io::PredefinedOids::kTidArray,
                                   io::PredefinedOids::kOidArray));
  EXPECT_FALSE(io::MappedToSameType(io::PredefinedOids::kTidArray,
                                    io::PredefinedOids::kInt8Array));

  EXPECT_FALSE(io::MappedToSameType(io::PredefinedOids::kOid,
                                    io::PredefinedOids::kOidArray));
  EXPECT_FALSE(io::MappedToSameType(io::PredefinedOids::kOidArray,
                                    io::PredefinedOids::kOid));
  EXPECT_FALSE(io::MappedToSameType(io::PredefinedOids::kTid,
                                    io::PredefinedOids::kTidArray));
}

USERVER_NAMESPACE_END
