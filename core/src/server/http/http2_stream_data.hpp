#pragma once

#include "http_request_constructor.hpp"

USERVER_NAMESPACE_BEGIN

namespace server::http {

struct Http2StreamData {
  Http2StreamData(const request::HttpRequestConfig& config,
                  const HandlerInfoIndex& handler_info_index,
                  request::ResponseDataAccounter& data_accounter,
                  uint32_t stream_id)
      : constructor(config, handler_info_index, data_accounter),
        stream_id(stream_id) {}
  HttpRequestConstructor constructor;
  uint32_t stream_id;
  // Useless
  int fd{-1};
};

}  // namespace server::http

USERVER_NAMESPACE_END
