/**
        curl-ev: wrapper for integrating libcurl with libev applications
        Copyright (c) 2013 Oliver Kuckertz <oliver.kuckertz@mologie.de>
        See COPYING for license information.
*/

#pragma once

#include <memory>

#include <engine/ev/thread.hpp>
#include <engine/ev/watcher/io_watcher.hpp>

namespace curl {

class easy;

struct socket_info : public std::enable_shared_from_this<socket_info> {
  explicit socket_info(engine::ev::ThreadControl& thread_control)
      : watcher(std::make_unique<engine::ev::IoWatcher>(thread_control)) {}

  bool IsEasySocket() const { return handle; }

  easy* handle{nullptr};
  std::unique_ptr<engine::ev::IoWatcher> watcher;
  bool pending_read_op{false};
  bool pending_write_op{false};
  bool monitor_read{false};
  bool monitor_write{false};
};
}  // namespace curl
