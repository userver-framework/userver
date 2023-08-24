#pragma once

/// @file userver/engine/io.hpp
/// @brief Include-all header for low-level asynchronous I/O interfaces

#include <userver/engine/io/buffered.hpp>
#include <userver/engine/io/exception.hpp>
#include <userver/engine/io/pipe.hpp>
#include <userver/engine/io/sockaddr.hpp>
#include <userver/engine/io/socket.hpp>

USERVER_NAMESPACE_BEGIN

/// Low-level asynchronous I/O interfaces
namespace engine::io {}

USERVER_NAMESPACE_END
