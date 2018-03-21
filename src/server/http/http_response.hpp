#pragma once

#include <chrono>
#include <string>
#include <unordered_map>

#include <engine/sender.hpp>
#include <server/request/response_base.hpp>
#include <utils/str_icase.hpp>

#include "http_status.hpp"

namespace server {
namespace http {

class HttpRequestImpl;

class HttpResponse : public request::ResponseBase {
 public:
  explicit HttpResponse(const HttpRequestImpl& request);
  virtual ~HttpResponse();

  void SetHeader(std::string name, std::string value);
  void SetContentType(std::string type);
  void SetContentEncoding(std::string encoding);
  void SetStatus(HttpStatus status);

  HttpStatus GetStatus() const { return status_; }

  virtual void SendResponse(engine::Sender& sender,
                            std::function<void(size_t)>&& finish_cb) override;

  virtual void SetStatusServiceUnavailable() override {
    SetStatus(HttpStatus::kServiceUnavailable);
  }
  virtual void SetStatusOk() override { SetStatus(HttpStatus::kOk); }
  virtual void SetStatusNotFound() override {
    SetStatus(HttpStatus::kNotFound);
  }

 private:
  std::string StatusString() const;

  const HttpRequestImpl& request_;
  HttpStatus status_ = HttpStatus::kOk;
  std::unordered_map<std::string, std::string, utils::StrIcaseHash,
                     utils::StrIcaseCmp>
      headers_;
};

}  // namespace http
}  // namespace server
