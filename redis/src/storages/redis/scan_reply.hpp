#pragma once

#include <redis/reply.hpp>

#include <storages/redis/parse_reply.hpp>
#include <storages/redis/reply_types.hpp>

namespace storages {
namespace redis {

template <ScanTag scan_tag>
class ScanReplyTmpl final {
 public:
  using ReplyElem = typename ScanReplyElem<scan_tag>::type;

  static ScanReplyTmpl Parse(const ReplyPtr& reply,
                             const std::string& request_description);

  class Cursor final {
   public:
    Cursor() : Cursor(0) {}
    uint64_t GetValue() const { return value_; }

    friend class ScanReplyTmpl;

   private:
    explicit Cursor(uint64_t value) : value_(value) {}

    uint64_t value_;
  };

  const Cursor& GetCursor() const { return cursor_; }

  const std::vector<ReplyElem>& GetKeys() const { return keys_; }

  std::vector<ReplyElem>& GetKeys() { return keys_; }

 private:
  ScanReplyTmpl(Cursor cursor, std::vector<ReplyElem> keys)
      : cursor_(cursor), keys_(std::move(keys)) {}

  Cursor cursor_;
  std::vector<ReplyElem> keys_;
};

template <ScanTag scan_tag>
ScanReplyTmpl<scan_tag> ScanReplyTmpl<scan_tag>::Parse(
    const ReplyPtr& reply, const std::string& request_description) {
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

  auto keys = ParseReplyDataArray(keys_elem, reply, request_description,
                                  To<std::vector<ReplyElem>>{});

  return ScanReplyTmpl{Cursor{cursor}, std::move(keys)};
}

using ScanReply = ScanReplyTmpl<ScanTag::kScan>;
using SscanReply = ScanReplyTmpl<ScanTag::kSscan>;
using HscanReply = ScanReplyTmpl<ScanTag::kHscan>;
using ZscanReply = ScanReplyTmpl<ScanTag::kZscan>;

}  // namespace redis
}  // namespace storages
