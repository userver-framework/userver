#pragma once

#include <mongocxx/cursor.hpp>

#include <engine/async.hpp>
#include <engine/task/task_processor.hpp>
#include "pool_impl.hpp"

namespace storages {
namespace mongo {
namespace impl {

class CursorImpl {
 public:
  // mongocxx::cursor::begin() may block
  CursorImpl(engine::TaskProcessor& task_processor,
             PoolImpl::ConnectionPtr&& conn, mongocxx::cursor&& cursor)
      : task_processor_(task_processor),
        conn_(std::move(conn)),
        cursor_(std::move(cursor)),
        it_(cursor_.begin()) {}

  bool IsExhausted() { return cursor_.begin() == cursor_.end(); }

  decltype(auto) operator*() const { return *it_; }
  decltype(auto) operator-> () const { return it_.operator->(); }

  CursorImpl& operator++() {
    engine::Async(task_processor_, [this] { ++it_; }).Get();
    return *this;
  }

 private:
  engine::TaskProcessor& task_processor_;
  const PoolImpl::ConnectionPtr conn_;
  mongocxx::cursor cursor_;
  mongocxx::cursor::iterator it_;
};

}  // namespace impl
}  // namespace mongo
}  // namespace storages
