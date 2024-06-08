#include <userver/ydb/io/insert_row.hpp>

#include <ydb-cpp-sdk/client/params/params.h>
#include <ydb-cpp-sdk/client/value/value.h>

#include <variant>

#include <userver/ydb/impl/cast.hpp>
#include <userver/ydb/io/primitives.hpp>
#include <userver/ydb/io/traits.hpp>
#include <userver/ydb/types.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb {

namespace {

template <typename Builder>
void WriteInsertRow(NYdb::TValueBuilderBase<Builder>& builder,
                    const InsertRow& value) {
  builder.BeginStruct();
  for (const InsertColumn& column : value) {
    builder.AddMember(impl::ToString(column.name));
    std::visit(
        [&builder](const auto& alternative) { Write(builder, alternative); },
        column.value);
  }
  builder.EndStruct();
}

}  // namespace

void ValueTraits<InsertRow>::Write(
    NYdb::TValueBuilderBase<NYdb::TValueBuilder>& builder,
    const InsertRow& value) {
  WriteInsertRow(builder, value);
}

void ValueTraits<InsertRow>::Write(
    NYdb::TValueBuilderBase<NYdb::TParamValueBuilder>& builder,
    const InsertRow& value) {
  WriteInsertRow(builder, value);
}

}  // namespace ydb

USERVER_NAMESPACE_END
