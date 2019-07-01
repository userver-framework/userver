#pragma once

namespace utils {

class OpensslLock final {
 public:
  static void Init();

 private:
  OpensslLock();

  static void LockingFunction(int mode, int n, const char* file, int line);
};

}  // namespace utils
