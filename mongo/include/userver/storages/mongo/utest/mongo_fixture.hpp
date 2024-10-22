#pragma once

/// @file userver/storages/mongo/utest/mongo_fixture.hpp
/// @brief @copybrief storages::mongo::MongoTest

#include <userver/utest/utest.hpp>

#include <userver/storages/mongo/utest/mongo_local.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::utest {

/// Mongo fixture
///
/// @brief Provides access to `storages::mongo::Pool` by means
/// `storages::mongo::utest::MongoLocal` class
///
/// see example: userver/samples/mongo_service/unittests
// NOLINTNEXTLINE(fuchsia-multiple-inheritance)
class MongoTest : public ::testing::Test, public MongoLocal {
public:
    MongoTest() { GetPool()->DropDatabase(); }
};

}  // namespace storages::mongo::utest

USERVER_NAMESPACE_END
