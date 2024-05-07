#pragma once

#include <userver/ydb/io/traits.hpp>
#include <userver/ydb/types.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb {

template <>
struct ValueTraits<InsertRow> {
  // We don't have enough type information to parse Null.
  static InsertRow Parse(NYdb::TValueParser& parser,
                         const std::string& column_name) = delete;

  static void Write(NYdb::TValueBuilderBase<NYdb::TValueBuilder>& builder,
                    const InsertRow& value);

  static void Write(NYdb::TValueBuilderBase<NYdb::TParamValueBuilder>& builder,
                    const InsertRow& value);
};

}  // namespace ydb

USERVER_NAMESPACE_END
