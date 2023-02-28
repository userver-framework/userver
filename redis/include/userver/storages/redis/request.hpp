#pragma once

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <userver/storages/redis/impl/exception.hpp>
#include <userver/storages/redis/reply_types.hpp>
#include <userver/storages/redis/request_data_base.hpp>
#include <userver/storages/redis/scan_tag.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

template <ScanTag scan_tag>
class RequestScanData;

template <typename Result, typename ReplyType = Result>
class [[nodiscard]] Request final {
 public:
  using Reply = ReplyType;

  explicit Request(std::unique_ptr<RequestDataBase<ReplyType>>&& impl)
      : impl_(std::move(impl)) {}

  void Wait() { impl_->Wait(); }

  void IgnoreResult() const {}

  ReplyType Get(const std::string& request_description = {}) {
    return impl_->Get(request_description);
  }

  template <typename T1, typename T2>
  friend class RequestEval;

  template <typename T1, typename T2>
  friend class RequestEvalSha;

  template <ScanTag scan_tag>
  friend class RequestScanData;

 private:
  ReplyPtr GetRaw() { return impl_->GetRaw(); }

  std::unique_ptr<RequestDataBase<ReplyType>> impl_;
};

template <ScanTag scan_tag>
class ScanRequest final {
 public:
  using ReplyElem = typename ScanReplyElem<scan_tag>::type;

  explicit ScanRequest(std::unique_ptr<RequestScanDataBase<scan_tag>>&& impl)
      : impl_(std::move(impl)) {}

  template <typename T = std::vector<ReplyElem>>
  T GetAll(std::string request_description) {
    SetRequestDescription(std::move(request_description));
    return GetAll<T>();
  }

  template <typename T = std::vector<ReplyElem>>
  T GetAll() {
    return T{begin(), end()};
  }

  void SetRequestDescription(std::string request_description) {
    impl_->SetRequestDescription(std::move(request_description));
  }

  class Iterator {
   public:
    using iterator_category = std::input_iterator_tag;
    using difference_type = ptrdiff_t;
    using value_type = ReplyElem;
    using reference = value_type&;
    using pointer = value_type*;

    explicit Iterator(ScanRequest* stream) : stream_(stream) {
      if (stream_ && !stream_->HasMore()) stream_ = nullptr;
    }

    class ReplyElemHolder {
     public:
      ReplyElemHolder(value_type reply_elem)
          : reply_elem_(std::move(reply_elem)) {}

      value_type& operator*() { return reply_elem_; }

     private:
      value_type reply_elem_;
    };

    ReplyElemHolder operator++(int) {
      ReplyElemHolder old_value(stream_->Current());
      ++*this;
      return old_value;
    }

    Iterator& operator++() {
      stream_->Get();
      if (!stream_->HasMore()) stream_ = nullptr;
      return *this;
    }

    reference operator*() { return stream_->Current(); }

    pointer operator->() { return &**this; }

    bool operator==(const Iterator& rhs) const {
      return stream_ == rhs.stream_;
    }

    bool operator!=(const Iterator& rhs) const { return !(*this == rhs); }

   private:
    ScanRequest* stream_;
  };

  Iterator begin() { return Iterator(this); }
  Iterator end() { return Iterator(nullptr); }

  class GetAfterEofException : public USERVER_NAMESPACE::redis::Exception {
   public:
    using USERVER_NAMESPACE::redis::Exception::Exception;
  };

 private:
  ReplyElem& Current() { return impl_->Current(); }

  ReplyElem Get() { return impl_->Get(); }

  bool HasMore() { return !impl_->Eof(); }

  friend class Iterator;

  std::unique_ptr<RequestScanDataBase<scan_tag>> impl_;
};

using RequestAppend = Request<size_t>;
using RequestDbsize = Request<size_t>;
using RequestDel = Request<size_t>;
using RequestUnlink = Request<size_t>;
using RequestEvalCommon = Request<ReplyData>;
using RequestEvalShaCommon = Request<ReplyData>;
using RequestScriptLoad = Request<std::string>;
using RequestExec = Request<ReplyData, void>;
using RequestExists = Request<size_t>;
using RequestExpire = Request<ExpireReply>;
using RequestGeoadd = Request<size_t>;
using RequestGeoradius = Request<std::vector<GeoPoint>>;
using RequestGeosearch = Request<std::vector<GeoPoint>>;
using RequestGet = Request<std::optional<std::string>>;
using RequestGetset = Request<std::optional<std::string>>;
using RequestHdel = Request<size_t>;
using RequestHexists = Request<size_t>;
using RequestHget = Request<std::optional<std::string>>;
using RequestHgetall = Request<std::unordered_map<std::string, std::string>>;
using RequestHincrby = Request<int64_t>;
using RequestHincrbyfloat = Request<double>;
using RequestHkeys = Request<std::vector<std::string>>;
using RequestHlen = Request<size_t>;
using RequestHmget = Request<std::vector<std::optional<std::string>>>;
using RequestHmset = Request<StatusOk, void>;
using RequestHscan = ScanRequest<ScanTag::kHscan>;
using RequestHset = Request<HsetReply>;
using RequestHsetnx = Request<size_t, bool>;
using RequestHvals = Request<std::vector<std::string>>;
using RequestIncr = Request<int64_t>;
using RequestKeys = Request<std::vector<std::string>>;
using RequestLindex = Request<std::optional<std::string>>;
using RequestLlen = Request<size_t>;
using RequestLpop = Request<std::optional<std::string>>;
using RequestLpush = Request<size_t>;
using RequestLpushx = Request<size_t>;
using RequestLrange = Request<std::vector<std::string>>;
using RequestLrem = Request<size_t>;
using RequestLtrim = Request<StatusOk, void>;
using RequestMget = Request<std::vector<std::optional<std::string>>>;
using RequestMset = Request<StatusOk, void>;
using RequestPersist = Request<PersistReply>;
using RequestPexpire = Request<ExpireReply>;
using RequestPing = Request<StatusPong, void>;
using RequestPingMessage = Request<std::string>;
using RequestPublish = Request<size_t>;
using RequestRename = Request<StatusOk, void>;
using RequestRpop = Request<std::optional<std::string>>;
using RequestRpush = Request<size_t>;
using RequestRpushx = Request<size_t>;
using RequestSadd = Request<size_t>;
using RequestScan = ScanRequest<ScanTag::kScan>;
using RequestScard = Request<size_t>;
using RequestSet = Request<StatusOk, void>;
using RequestSetIfExist = Request<std::optional<StatusOk>, bool>;
using RequestSetIfNotExist = Request<std::optional<StatusOk>, bool>;
using RequestSetOptions = Request<SetReply>;
using RequestSetex = Request<StatusOk, void>;
using RequestSismember = Request<size_t>;
using RequestSmembers = Request<std::unordered_set<std::string>>;
using RequestSrandmember = Request<std::optional<std::string>>;
using RequestSrandmembers = Request<std::vector<std::string>>;
using RequestSrem = Request<size_t>;
using RequestSscan = ScanRequest<ScanTag::kSscan>;
using RequestStrlen = Request<size_t>;
using RequestTime = Request<std::chrono::system_clock::time_point>;
using RequestTtl = Request<TtlReply>;
using RequestType = Request<KeyType>;
using RequestZadd = Request<size_t>;
using RequestZaddIncr = Request<double>;
using RequestZaddIncrExisting = Request<std::optional<double>>;
using RequestZcard = Request<size_t>;
using RequestZrange = Request<std::vector<std::string>>;
using RequestZrangeWithScores = Request<std::vector<MemberScore>>;
using RequestZrangebyscore = Request<std::vector<std::string>>;
using RequestZrangebyscoreWithScores = Request<std::vector<MemberScore>>;
using RequestZrem = Request<size_t>;
using RequestZremrangebyrank = Request<size_t>;
using RequestZremrangebyscore = Request<size_t>;
using RequestZscan = ScanRequest<ScanTag::kZscan>;
using RequestZscore = Request<std::optional<double>>;

}  // namespace storages::redis

USERVER_NAMESPACE_END
