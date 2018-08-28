#pragma once

#include <chrono>
#include <string>
#include <unordered_map>

#include <boost/range/adaptor/map.hpp>

#include <server/request/response_base.hpp>
#include <utils/str_icase.hpp>

#include "http_status.hpp"

namespace engine {
class Sender;
}  // namespace engine

namespace server {
namespace http {

class HttpRequestImpl;

class HttpResponse : public request::ResponseBase {
 public:
  using HeadersMap =
      std::unordered_map<std::string, std::string, utils::StrIcaseHash,
                         utils::StrIcaseCmp>;

  using HeadersMapKeys = decltype(HeadersMap() | boost::adaptors::map_keys);

  explicit HttpResponse(const HttpRequestImpl& request);
  virtual ~HttpResponse();

  virtual void SetSent(size_t bytes_sent) override;

  void SetHeader(std::string name, std::string value);
  void SetContentType(std::string type);
  void SetContentEncoding(std::string encoding);
  void SetStatus(HttpStatus status);
  void ClearHeaders();

  HttpStatus GetStatus() const { return status_; }

  virtual void SendResponse(engine::Sender& sender,
                            std::function<void(size_t)> finish_cb,
                            bool need_send) override;

  virtual void SetStatusServiceUnavailable() override {
    SetStatus(HttpStatus::kServiceUnavailable);
  }
  virtual void SetStatusOk() override { SetStatus(HttpStatus::kOk); }
  virtual void SetStatusNotFound() override {
    SetStatus(HttpStatus::kNotFound);
  }

  HeadersMapKeys GetHeaderNames() const;
  const std::string& GetHeader(const std::string& header_name) const;

 private:
  std::string StatusString() const;

  const HttpRequestImpl& request_;
  HttpStatus status_ = HttpStatus::kOk;
  HeadersMap headers_;
};

}  // namespace http
}  // namespace server
