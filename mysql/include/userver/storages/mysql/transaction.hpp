#pragma once

/// @file userver/storages/mysql/transaction.hpp

#include <userver/tracing/span.hpp>
#include <userver/utils/fast_pimpl.hpp>

#include <userver/storages/mysql/client_interface.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql {

namespace infra {
class ConnectionPtr;
}

/// @brief RAII transaction wrapper, auto-<b>ROLLBACK</b>s on destruction if no
/// prior `Commit`/`Rollback` call was made.
///
/// This type can't be constructed in user code and is always retrieved from
/// storages::mysql::Cluster
class Transaction final : public IClientInterface {
 public:
  explicit Transaction(infra::ConnectionPtr&& connection,
                       engine::Deadline deadline);
  ~Transaction();
  Transaction(const Transaction& other) = delete;
  Transaction(Transaction&& other) noexcept;

  template <typename... Args>
  StatementResultSet Execute(const Query& query, const Args&... args) const;

  /// @brief Commit the transaction
  void Commit();

  /// @brief Rollback the transaction
  void Rollback();

 private:
  StatementResultSet DoExecute(
      OptionalCommandControl command_control, ClusterHostType host_type,
      const Query& query, impl::io::ParamsBinderBase& params,
      std::optional<std::size_t> batch_size) const override;

  void AssertValid() const;

  utils::FastPimpl<infra::ConnectionPtr, 24, 8> connection_;
  engine::Deadline deadline_;
  tracing::Span span_;
};

template <typename... Args>
StatementResultSet Transaction::Execute(const Query& query,
                                        const Args&... args) const {
  auto params_binder = impl::BindHelper::BindParams(args...);

  return DoExecute({},
                   /* doesn't matter here, just conforming to the interface*/
                   ClusterHostType::kMaster, query, params_binder,
                   std::nullopt);
}

}  // namespace storages::mysql

USERVER_NAMESPACE_END
