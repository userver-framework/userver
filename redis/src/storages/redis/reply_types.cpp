#include <storages/redis/reply_types.hpp>

#include <redis/reply/scan_reply.hpp>

namespace storages {
namespace redis {

ScanReply ScanReply::Parse(::redis::ReplyPtr reply,
                           const std::string& request_description) {
  reply->ExpectArray(request_description);
  const auto& request =
      request_description.empty() ? reply->cmd : request_description;

  const auto& data = reply->data;

  const auto& top_array = data.GetArray();
  if (top_array.size() != 2) {
    throw ::redis::ParseReplyException(
        "Unexpected reply size to '" + request +
        "' request: expected 2 elements, received " +
        std::to_string(top_array.size()));
  }

  const auto& cursor_elem = top_array[0];
  if (!cursor_elem.IsString()) {
    throw ::redis::ParseReplyException(
        "Unexpected format of reply to '" + request + "' request: expected " +
        ::redis::ReplyData::TypeToString(::redis::ReplyData::Type::kString) +
        " as first element, received " + cursor_elem.GetTypeString());
  }

  const auto& keys_elem = top_array[1];
  if (!keys_elem.IsArray()) {
    throw ::redis::ParseReplyException(
        "Unexpected format of reply to '" + request + "' request: expected " +
        ::redis::ReplyData::TypeToString(::redis::ReplyData::Type::kArray) +
        " as second element, received " + keys_elem.GetTypeString());
  }

  uint64_t cursor;
  try {
    cursor = std::stoul(cursor_elem.GetString());
  } catch (const std::exception& ex) {
    throw ::redis::ParseReplyException(
        "Can't parse reply to '" + request +
        "' request: " + "Can't parse cursor from " + cursor_elem.GetString());
  }
  const auto& keys_data = keys_elem.GetArray();

  std::vector<std::string> keys;
  keys.reserve(keys_data.size());
  for (const auto& key_data : keys_data) {
    if (!key_data.IsString()) {
      throw ::redis::ParseReplyException(
          "Unexpected format of reply to '" + request +
          "' request: expected keys of type " +
          ::redis::ReplyData::TypeToString(::redis::ReplyData::Type::kString) +
          ", but one of elements has " + key_data.GetTypeString() + " type");
    }
    keys.emplace_back(key_data.GetString());
  }

  return ScanReply{Cursor{cursor}, std::move(keys)};
}

ScanReply::ScanReply(Cursor cursor, std::vector<std::string> keys)
    : cursor_(cursor), keys_(std::move(keys)) {}

}  // namespace redis
}  // namespace storages
