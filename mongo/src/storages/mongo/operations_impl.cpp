#include <storages/mongo/operations_impl.hpp>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::operations {

void AppendComment(formats::bson::impl::BsonBuilder& builder,
                   bool& has_comment_option, const options::Comment& comment) {
  if (comment.Value().empty()) return;

  UASSERT(!has_comment_option);
  has_comment_option = true;

  static const std::string kOptionName = "comment";
  builder.Append(kOptionName, comment.Value());
}

void AppendMaxServerTime(formats::bson::impl::BsonBuilder& builder,
                         bool& has_max_server_time_option,
                         const options::MaxServerTime& max_server_time) {
  UASSERT(!has_max_server_time_option);
  has_max_server_time_option = true;

  static const std::string kOptionName = "maxTimeMS";
  builder.Append(kOptionName, max_server_time.Value().count());
}

}  // namespace storages::mongo::operations

USERVER_NAMESPACE_END
