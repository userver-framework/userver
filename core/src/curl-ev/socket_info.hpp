/**
        curl-ev: wrapper for integrating libcurl with libev applications
        Copyright (c) 2013 Oliver Kuckertz <oliver.kuckertz@mologie.de>
        See COPYING for license information.
*/

#pragma once

#include <engine/ev/thread_control.hpp>
#include <engine/ev/watcher/io_watcher.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace curl {

class easy;

struct socket_info {
  explicit socket_info(engine::ev::ThreadControl& thread_control)
      : watcher(thread_control) {}

  ~socket_info() { UASSERT(!handle); }

  bool IsEasySocket() const { return handle; }

  easy* handle{nullptr};
  engine::ev::IoWatcher watcher;
  bool pending_read_op{false};
  bool pending_write_op{false};
  bool monitor_read{false};
  bool monitor_write{false};
};
}  // namespace curl

USERVER_NAMESPACE_END
