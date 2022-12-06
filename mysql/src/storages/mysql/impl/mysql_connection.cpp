#include <storages/mysql/impl/mysql_connection.hpp>

#include <stdexcept>

#include <userver/engine/task/task.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl {

namespace {

MYSQL InitMysql() {
  MYSQL mysql;
  mysql_init(&mysql);
  mysql_options(&mysql, MYSQL_OPT_NONBLOCK, nullptr);

  return mysql;
}

template <typename StartFn, typename ContFn>
void RunToCompletion(MySQLSocket& socket_, StartFn&& start_fn, ContFn cont_fn,
                     engine::Deadline deadline) {
  socket_.SetEvents(start_fn());
  while (socket_.ShouldWait()) {
    const auto mysql_events = socket_.Wait(deadline);
    socket_.SetEvents(cont_fn(mysql_events));
  }
}

class MySQLResultDeleter final {
 public:
  MySQLResultDeleter(MySQLSocket& socket) : socket_{socket} {}

  void operator()(MYSQL_RES* res) noexcept {
    try {
      RunToCompletion(
          socket_, [res] { return mysql_free_result_start(res); },
          [res](int mysql_events) {
            return mysql_free_result_cont(res, mysql_events);
          },
          {} /* TODO : deadline? */);
    } catch (const std::exception& ex) {
    }
  }

 private:
  MySQLSocket& socket_;
};

}  // namespace

MySQLConnection::MySQLConnection()
    : mysql_{InitMysql()}, socket_{InitSocket()} {
  while (socket_.ShouldWait()) {
    const auto mysql_events = socket_.Wait({});
    socket_.SetEvents(
        mysql_real_connect_cont(&connect_ret_, &mysql_, mysql_events));
  }

  if (!connect_ret_) {
    throw std::runtime_error{std::string{"Failed to connect: "} +
                             mysql_error(&mysql_)};
  }
}

MySQLConnection::~MySQLConnection() {
  RunToCompletion(
      socket_, [this] { return mysql_close_start(&mysql_); },
      [this](int mysql_event) {
        return mysql_close_cont(&mysql_, mysql_event);
      },
      {});
}

MySQLResult MySQLConnection::Execute(const std::string& query,
                                     engine::Deadline deadline) {
  MySQLExecuteQuery(query, deadline);

  return MySQLFetchResult(deadline);
}

MySQLSocket MySQLConnection::InitSocket() {
  const auto mysql_events = mysql_real_connect_start(
      &connect_ret_, &mysql_, "0.0.0.0", "test", "test", "test", 0 /* port */,
      nullptr /* unix_socket */, 0 /* client_flags */);

  return MySQLSocket{mysql_get_socket(&mysql_), mysql_events};
}

void MySQLConnection::MySQLExecuteQuery(const std::string& query,
                                        engine::Deadline deadline) {
  int err = 0;
  RunToCompletion(
      socket_,
      [this, &err, &query] {
        return mysql_real_query_start(&err, &mysql_, query.data(),
                                      query.size());
      },
      [this, &err](int mysql_events) {
        return mysql_real_query_cont(&err, &mysql_, mysql_events);
      },
      deadline);

  if (err != 0) {
    throw std::runtime_error{std::string{"Failed to execute query: "} +
                             mysql_error(&mysql_)};
  }
}

MySQLResult MySQLConnection::MySQLFetchResult(engine::Deadline deadline) {
  std::unique_ptr<MYSQL_RES, MySQLResultDeleter> res{
      mysql_use_result(&mysql_), MySQLResultDeleter{socket_}};
  if (!res) {
    throw std::runtime_error{std::string{"Idk, TODO"} + mysql_error(&mysql_)};
  }

  MySQLResult result{};
  MYSQL_ROW row{nullptr};
  while (true) {
    RunToCompletion(
        socket_,
        [&row, res = res.get()] { return mysql_fetch_row_start(&row, res); },
        [&row, res = res.get()](int mysql_events) {
          return mysql_fetch_row_cont(&row, res, mysql_events);
        },
        deadline);

    if (!row) break;

    result.AppendRow(MySQLRow{row, mysql_field_count(&mysql_)});
  }

  return result;
}

}  // namespace storages::mysql::impl

USERVER_NAMESPACE_END
