#include <engine/io/error.hpp>

namespace engine {
namespace io {

IoError::IoError() : std::runtime_error("generic I/O error") {}

IoTimeout::IoTimeout() : IoTimeout(0) {}

IoTimeout::IoTimeout(size_t bytes_transferred)
    : IoError("I/O operation timed out"),
      bytes_transferred_(bytes_transferred) {}

}  // namespace io
}  // namespace engine
