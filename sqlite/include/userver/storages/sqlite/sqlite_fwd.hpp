#pragma once

/// @file userver/storages/sqlite/sqlite_fwd.hpp
/// @brief Forward declarations of some popular sqlite related types

#include <memory>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite {

class Transaction;
class Savepoint;
class ResultSet;
struct ExecutionResult;

class Client;
using ClientPtr = std::shared_ptr<Client>;

namespace impl {
class Connection;

class ClientImpl;
using ClientImplPtr = std::unique_ptr<ClientImpl>;

class StatementBase;
using StatementBasePtr = std::shared_ptr<StatementBase>;

class Statement;
using StatementPtr = std::shared_ptr<Statement>;

class ResultWrapper;
using ResultWrapperPtr = std::unique_ptr<impl::ResultWrapper>;
}  // namespace impl

namespace infra {

class ConnectionPtr;

class Pool;
using PoolPtr = std::shared_ptr<Pool>;

namespace strategy {
class PoolStrategyBase;
using PoolStrategyBasePtr = std::unique_ptr<PoolStrategyBase>;
}  // namespace strategy

}  // namespace infra

}  // namespace storages::sqlite

USERVER_NAMESPACE_END
