#include <userver/utest/utest.hpp>
#include "../utils_mysqltest.hpp"

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::tests {

namespace {

template <typename DateType>
void ValidateDate(TmpTable& table, const DateType& date) {
  table.DefaultExecute("INSERT INTO {} VALUES(?)", date);

  // system_clock::time_point::duration is microseconds on apple,
  // so these don't throw
#if !defined(__APPLE__)
  UEXPECT_THROW(date.ToTimePoint(), mysql::MySQLValidationException);

  UEXPECT_THROW(table.DefaultExecute("SELECT value FROM {}")
                    .AsSingleField<std::chrono::system_clock::time_point>(),
                mysql::MySQLValidationException);
#endif

  UEXPECT_NO_THROW(
      table.DefaultExecute("SELECT value FROM {}").AsSingleField<DateType>());
}

}  // namespace

UTEST(Date, OutOfSystemTimepointRange) {
  TmpTable table{"value DATE NOT NULL"};

  const Date date{1500, 11, 2};
  ValidateDate(table, date);

  const Date valid_date{1971, 1, 2};
  UEXPECT_NO_THROW(valid_date.ToTimePoint());
}

UTEST(DateTime, OutOfSystemTimepointRange) {
  TmpTable table{"value DATETIME(6) NOT NULL"};

  const DateTime datetime{1500, 11, 2, 14, 17, 31, 123019};
  ValidateDate(table, datetime);

  const DateTime valid_datetime{2023, 4, 10, 20, 12, 7, 112358};
  UEXPECT_NO_THROW(valid_datetime.ToTimePoint());
}

}  // namespace storages::mysql::tests

USERVER_NAMESPACE_END
