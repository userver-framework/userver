#include "log.hpp"

namespace logging {

LoggerPtr& Log() {
  static LoggerPtr logger = MakeStderrLogger("default");
  return logger;
}

}  // namespace logging
