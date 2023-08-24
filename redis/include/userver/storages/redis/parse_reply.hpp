#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <userver/storages/redis/impl/types.hpp>
#include <userver/utils/void_t.hpp>

#include <userver/storages/redis/reply_types.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {
namespace impl {

ReplyData&& ExtractData(ReplyPtr& reply);

template <typename Result, typename ReplyType, typename = utils::void_t<>>
struct HasParseFunctionFromRedisReply {
  static constexpr bool value = false;
};

template <typename Result, typename ReplyType>
struct HasParseFunctionFromRedisReply<
    Result, ReplyType,
    utils::void_t<decltype(Result::Parse(
        std::declval<ReplyData>(), std::declval<const std::string&>()))>> {
  static constexpr bool value =
      std::is_same<decltype(Result::Parse(std::declval<ReplyData>(),
                                          std::declval<const std::string&>())),
                   ReplyType>::value;
};

bool IsNil(const ReplyData& reply_data);

void ExpectIsOk(const ReplyPtr& reply, const std::string& request_description);

void ExpectArray(const ReplyData& reply_data,
                 const std::string& request_description);

const std::string& RequestDescription(const ReplyPtr& reply,
                                      const std::string& request_description);

}  // namespace impl

/// An ADL helper that allows searching for `Parse` functions in namespace
/// `storages::redis` additionally to the namespace of `Result`.
template <typename Result, typename ReplyType = Result>
struct To {};

std::vector<std::string> ParseReplyDataArray(
    ReplyData&& array_data, const std::string& request_description,
    To<std::vector<std::string>>);

std::vector<std::optional<std::string>> ParseReplyDataArray(
    ReplyData&& array_data, const std::string& request_description,
    To<std::vector<std::optional<std::string>>>);

std::vector<std::pair<std::string, std::string>> ParseReplyDataArray(
    ReplyData&& array_data, const std::string& request_description,
    To<std::vector<std::pair<std::string, std::string>>>);

std::vector<MemberScore> ParseReplyDataArray(
    ReplyData&& array_data, const std::string& request_description,
    To<std::vector<MemberScore>>);

std::vector<GeoPoint> ParseReplyDataArray(
    ReplyData&& array_data, const std::string& request_description,
    To<std::vector<GeoPoint>>);

std::string Parse(ReplyData&& reply_data,
                  const std::string& request_description, To<std::string>);

double Parse(ReplyData&& reply_data, const std::string& request_description,
             To<double>);

size_t Parse(ReplyData&& reply_data, const std::string& request_description,
             To<size_t>);

bool Parse(ReplyData&& reply_data, const std::string& request_description,
           To<size_t, bool>);

int64_t Parse(ReplyData&& reply_data, const std::string& request_description,
              To<int64_t>);

std::chrono::system_clock::time_point Parse(
    ReplyData&& reply_data, const std::string& request_description,
    To<std::chrono::system_clock::time_point>);

HsetReply Parse(ReplyData&& reply_data, const std::string& request_description,
                To<HsetReply>);

PersistReply Parse(ReplyData&& reply_data,
                   const std::string& request_description, To<PersistReply>);

KeyType Parse(ReplyData&& reply_data, const std::string& request_description,
              To<KeyType>);

void Parse(ReplyData&& reply_data, const std::string& request_description,
           To<StatusOk, void>);

bool Parse(ReplyData&& reply_data, const std::string& request_description,
           To<std::optional<StatusOk>, bool>);

void Parse(ReplyData&& reply_data, const std::string& request_description,
           To<StatusPong, void>);

SetReply Parse(ReplyData&& reply_data, const std::string& request_description,
               To<SetReply>);

std::unordered_set<std::string> Parse(ReplyData&& reply_data,
                                      const std::string& request_description,
                                      To<std::unordered_set<std::string>>);

std::unordered_map<std::string, std::string> Parse(
    ReplyData&& reply_data, const std::string& request_description,
    To<std::unordered_map<std::string, std::string>>);

ReplyData Parse(ReplyData&& reply_data, const std::string& request_description,
                To<ReplyData>);

template <typename Result, typename ReplyType = Result>
std::enable_if_t<impl::HasParseFunctionFromRedisReply<Result, ReplyType>::value,
                 ReplyType>
Parse(ReplyData&& reply_data, const std::string& request_description,
      To<Result, ReplyType>) {
  return Result::Parse(std::move(reply_data), request_description);
}

template <typename T>
std::vector<T> Parse(ReplyData&& reply_data,
                     const std::string& request_description,
                     To<std::vector<T>>) {
  impl::ExpectArray(reply_data, request_description);
  return ParseReplyDataArray(std::move(reply_data), request_description,
                             To<std::vector<T>>{});
}

template <typename T>
std::optional<T> Parse(ReplyData&& reply_data,
                       const std::string& request_description,
                       To<std::optional<T>>) {
  if (impl::IsNil(reply_data)) return std::nullopt;
  return Parse(std::move(reply_data), request_description, To<T>{});
}

template <typename Result, typename ReplyType = Result>
ReplyType ParseReply(ReplyPtr reply,
                     const std::string& request_description = {}) {
  const auto& description =
      impl::RequestDescription(reply, request_description);
  impl::ExpectIsOk(reply, description);
  return Parse(impl::ExtractData(reply), description, To<Result, ReplyType>{});
}

}  // namespace storages::redis

USERVER_NAMESPACE_END
