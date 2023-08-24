#include "scan_reply.hpp"

#include <userver/storages/redis/impl/exception.hpp>

USERVER_NAMESPACE_BEGIN

namespace redis {

ScanReply ScanReply::parse(ReplyPtr reply) {
  reply->ExpectArray();

  const ReplyData& data = reply->data;

  const ReplyData::Array& top_array = data.GetArray();
  if (top_array.size() != 2) {
    throw ParseReplyException(
        "Unexpected SCAN reply size: expected 2 elements, received " +
        std::to_string(top_array.size()));
  }

  const ReplyData& cursor_elem = top_array[0];
  if (!cursor_elem.IsInt()) {
    throw ParseReplyException(
        "Unexpected SCAN reply format: expected " +
        ReplyData::TypeToString(ReplyData::Type::kInteger) +
        " as first element, received " + cursor_elem.GetTypeString());
  }

  const ReplyData& keys_elem = top_array[1];
  if (!keys_elem.IsArray()) {
    throw ParseReplyException("Unexpected SCAN reply format: expected " +
                              ReplyData::TypeToString(ReplyData::Type::kArray) +
                              " as second element, received " +
                              keys_elem.GetTypeString());
  }

  const ScanCursor cursor = cursor_elem.GetInt();
  const ReplyData::Array& keys_data = keys_elem.GetArray();

  std::vector<std::string> keys;
  keys.reserve(keys_data.size());
  for (const ReplyData& key_data : keys_data) {
    if (!key_data.IsString()) {
      throw ParseReplyException(
          "Unexpected SCAN reply format: expected keys of type " +
          ReplyData::TypeToString(ReplyData::Type::kString) +
          ", but one of elements has " + key_data.GetTypeString() + " type");
    }
    keys.emplace_back(key_data.GetString());
  }

  ScanReply result;
  result.cursor = cursor == 0 ? std::nullopt : std::make_optional(cursor);
  result.keys = std::move(keys);
  return result;
}

}  // namespace redis

USERVER_NAMESPACE_END
