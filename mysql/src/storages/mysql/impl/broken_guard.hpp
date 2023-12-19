#pragma once

#include <string>

#include <userver/storages/mysql/exceptions.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl {

class Connection;

class BrokenGuard final {
 public:
  explicit BrokenGuard(Connection& connection);
  ~BrokenGuard();

  template <typename Func>
  auto Execute(Func&& func);

 private:
  Connection& connection_;

  int exceptions_on_enter_;
  unsigned int errno_{0};
};

template <typename Func>
auto BrokenGuard::Execute(Func&& func) {
  try {
    return func();
  } catch (const MySQLException& ex) {
    errno_ = ex.GetErrno();
    throw;
  }
}

}  // namespace storages::mysql::impl

USERVER_NAMESPACE_END
