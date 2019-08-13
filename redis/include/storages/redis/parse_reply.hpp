#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <boost/optional.hpp>

#include <redis/types.hpp>
#include <utils/void_t.hpp>

#include <storages/redis/reply_types.hpp>

namespace redis {
class ReplyData;
}  // namespace redis

namespace storages {
namespace redis {
namespace impl {

const ::redis::ReplyData& GetArrayData(const ReplyPtr& reply,
                                       const std::string& request_description);

template <typename Result, typename ReplyType, typename = ::utils::void_t<>>
struct HasParseFunctionFromRedisReply {
  static constexpr bool value = false;
};

template <typename Result, typename ReplyType>
struct HasParseFunctionFromRedisReply<
    Result, ReplyType,
    ::utils::void_t<decltype(
        Result::Parse(std::declval<const ReplyPtr&>(),
                      std::declval<const std::string&>()))>> {
  static constexpr bool value =
      std::is_same<decltype(Result::Parse(std::declval<const ReplyPtr&>(),
                                          std::declval<const std::string&>())),
                   ReplyType>::value;
};

}  // namespace impl

/// An ADL helper that allows searching for `Parse` functions in namespace
/// `storages::redis` additionally to the namespace of `Result`.
template <typename Result, typename ReplyType = impl::DefaultReplyType<Result>>
struct To {};

std::vector<std::string> ParseReplyDataArray(
    const ::redis::ReplyData& array_data, const ::redis::ReplyPtr& reply,
    const std::string& request_description, To<std::vector<std::string>>);

std::vector<boost::optional<std::string>> ParseReplyDataArray(
    const ::redis::ReplyData& array_data, const ::redis::ReplyPtr& reply,
    const std::string& request_description,
    To<std::vector<boost::optional<std::string>>>);

std::vector<std::pair<std::string, std::string>> ParseReplyDataArray(
    const ::redis::ReplyData& array_data, const ::redis::ReplyPtr& reply,
    const std::string& request_description,
    To<std::vector<std::pair<std::string, std::string>>>);

std::vector<MemberScore> ParseReplyDataArray(
    const ::redis::ReplyData& array_data, const ::redis::ReplyPtr& reply,
    const std::string& request_description, To<std::vector<MemberScore>>);

std::string Parse(const ReplyPtr& reply, const std::string& request_description,
                  To<std::string>);

boost::optional<std::string> Parse(const ReplyPtr& reply,
                                   const std::string& request_description,
                                   To<boost::optional<std::string>>);

double Parse(const ReplyPtr& reply, const std::string& request_description,
             To<double>);

boost::optional<double> Parse(const ReplyPtr& reply,
                              const std::string& request_description,
                              To<boost::optional<double>>);

size_t Parse(const ReplyPtr& reply, const std::string& request_description,
             To<size_t>);

bool Parse(const ReplyPtr& reply, const std::string& request_description,
           To<size_t, bool>);

int64_t Parse(const ReplyPtr& reply, const std::string& request_description,
              To<int64_t>);

HsetReply Parse(const ReplyPtr& reply, const std::string& request_description,
                To<HsetReply>);

PersistReply Parse(const ReplyPtr& reply,
                   const std::string& request_description, To<PersistReply>);

KeyType Parse(const ReplyPtr& reply, const std::string& request_description,
              To<KeyType>);

void Parse(const ReplyPtr& reply, const std::string& request_description,
           To<StatusOk, void>);

bool Parse(const ReplyPtr& reply, const std::string& request_description,
           To<boost::optional<StatusOk>, bool>);

void Parse(const ReplyPtr& reply, const std::string& request_description,
           To<StatusPong, void>);

SetReply Parse(const ReplyPtr& reply, const std::string& request_description,
               To<SetReply>);

std::unordered_set<std::string> Parse(const ReplyPtr& reply,
                                      const std::string& request_description,
                                      To<std::unordered_set<std::string>>);

std::unordered_map<std::string, std::string> Parse(
    const ReplyPtr& reply, const std::string& request_description,
    To<std::unordered_map<std::string, std::string>>);

::redis::ReplyData Parse(const ReplyPtr& reply,
                         const std::string& request_description,
                         To<::redis::ReplyData>);

template <typename Result, typename ReplyType = impl::DefaultReplyType<Result>>
std::enable_if_t<impl::HasParseFunctionFromRedisReply<Result, ReplyType>::value,
                 ReplyType>
Parse(const ReplyPtr& reply, const std::string& request_description,
      To<Result, ReplyType>) {
  return Result::Parse(reply, request_description);
}

template <typename T>
std::vector<T> Parse(const ReplyPtr& reply,
                     const std::string& request_description,
                     To<std::vector<T>>) {
  return ParseReplyDataArray(impl::GetArrayData(reply, request_description),
                             reply, request_description, To<std::vector<T>>{});
}

const std::string& RequestDescription(const ReplyPtr& reply,
                                      const std::string& request_description);

template <typename Result, typename ReplyType = impl::DefaultReplyType<Result>>
ReplyType ParseReply(const ReplyPtr& reply,
                     const std::string& request_description = {}) {
  return Parse(reply, RequestDescription(reply, request_description),
               To<Result, ReplyType>{});
}

}  // namespace redis
}  // namespace storages
