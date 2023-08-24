#pragma once

#include <libpq-fe.h>
#include <memory>
#include <string_view>

#include <userver/storages/postgres/postgres_fwd.hpp>

#include <userver/logging/log_extra.hpp>
#include <userver/storages/postgres/message.hpp>
#include <userver/storages/postgres/result_set.hpp>
#include <userver/storages/postgres/sql_state.hpp>
#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::detail {

/// @brief Wrapper for PGresult
class ResultWrapper {
 public:
  using ResultHandle = std::unique_ptr<PGresult, decltype(&PQclear)>;

  ResultWrapper(ResultHandle&& res);
  ~ResultWrapper();

  void FillBufferCategories(const UserTypes& types);

  ExecStatusType GetStatus() const;

  //@{
  /** @name Data result */
  std::size_t RowCount() const;
  std::size_t FieldCount() const;
  const io::TypeBufferCategory& GetTypeBufferCategories() const {
    return buffer_categories_;
  }
  void SetTypeBufferCategories(const io::TypeBufferCategory& cats) {
    buffer_categories_ = cats;
  }
  std::string CommandStatus() const;
  std::size_t RowsAffected() const;

  std::size_t IndexOfName(const std::string& name) const;

  std::string_view GetFieldName(std::size_t col) const;
  FieldDescription GetFieldDescription(std::size_t col) const;
  Oid GetFieldTypeOid(std::size_t col) const;
  io::BufferCategory GetFieldBufferCategory(std::size_t col) const;
  bool IsFieldNull(std::size_t row, std::size_t col) const;
  std::size_t GetFieldLength(std::size_t row, std::size_t col) const;
  io::FieldBuffer GetFieldBuffer(std::size_t row, std::size_t col) const;
  //@}

  //@{
  /** @name Message result */
  // TODO Consider splitting these methods into a separate class
  std::string GetErrorMessage() const;
  std::string GetPrimaryErrorMessage() const;
  std::string GetDetailErrorMessage() const;
  std::string GetMessageSeverityString() const;
  Message::Severity GetMessageSeverity() const;
  std::string GetSqlCode() const;
  SqlState GetSqlState() const;

  std::string GetMessageSchema() const;
  std::string GetMessageTable() const;
  std::string GetMessageColumn() const;
  std::string GetMessageDatatype() const;
  std::string GetMessageConstraint() const;

  std::string GetMessageField(int fieldcode) const;

  logging::LogExtra GetMessageLogExtra() const;
  //@}

  ResultHandle handle_;
  io::TypeBufferCategory buffer_categories_;

  struct CachedFieldBufferCategories;
  USERVER_NAMESPACE::utils::FastPimpl<CachedFieldBufferCategories, 88, 8>
      cached_buffer_categories_;
};

inline ResultWrapper::ResultHandle MakeResultHandle(PGresult* pg_res) {
  return {pg_res, &PQclear};
}

}  // namespace storages::postgres::detail

USERVER_NAMESPACE_END
