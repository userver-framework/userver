#include <storages/mongo/operations_impl.hpp>

#include <userver/storages/mongo/exception.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::operations {

stats::OpType ToStatsOpType(operations::Update::Mode mode) {
  switch (mode) {
    case operations::Update::Mode::kSingle:
      return stats::OpType::kUpdateOne;
    case operations::Update::Mode::kMulti:
      return stats::OpType::kUpdateMany;
  }
  UINVARIANT(false, "Invalid value of operations::Update::Mode enum");
}

stats::OpType ToStatsOpType(operations::Delete::Mode mode) {
  switch (mode) {
    case operations::Delete::Mode::kSingle:
      return stats::OpType::kDeleteOne;
    case operations::Delete::Mode::kMulti:
      return stats::OpType::kDeleteMany;
  }
  UINVARIANT(false, "Invalid value of operations::Delete::Mode enum");
}

void AppendComment(formats::bson::impl::BsonBuilder& builder,
                   bool& has_comment_option, const options::Comment& comment) {
  if (comment.Value().empty()) return;

  UASSERT(!has_comment_option);
  has_comment_option = true;

  constexpr std::string_view kOptionName = "comment";
  builder.Append(kOptionName, comment.Value());
}

void AppendMaxServerTime(std::chrono::milliseconds& destination,
                         const options::MaxServerTime& max_server_time) {
  UASSERT_MSG(destination == kNoMaxServerTime,
              "Duplicate options::MaxServerTime");
  // Verification is delayed to Execute stage to handle errors correctly.
  destination = max_server_time.Value();
}

void VerifyMaxServerTime(std::chrono::milliseconds& max_server_time) {
  if (max_server_time.count() > std::numeric_limits<uint32_t>::max()) {
    // far future MaxServerTime ~= no MaxServerTime
    max_server_time = kNoMaxServerTime;
    return;
  }

  if (max_server_time < kNoMaxServerTime) {
    throw InvalidQueryArgumentException("Max server time of ")
        << max_server_time.count() << "ms is out of bounds";
  }
}

}  // namespace storages::mongo::operations

USERVER_NAMESPACE_END
