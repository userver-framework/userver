#include <utils/statistics/system_statistics.hpp>

#ifdef __APPLE__
#include <mach/mach.h>
#include <sys/resource.h>
#include <sys/time.h>
#endif
#include <unistd.h>

#include <algorithm>
#include <iterator>
#include <string>
#include <string_view>

#include <fmt/format.h>
#include <boost/filesystem/operations.hpp>
#include <boost/pfr/core.hpp>
#include <boost/system/error_code.hpp>

#include <userver/fs/blocking/read.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/from_string.hpp>
#include <userver/utils/statistics/writer.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics::impl {

namespace {

#ifdef __linux__
template <typename T>
void MergeSingleStat(std::optional<T>& to, std::optional<T> from) {
  if (!from) return;
  if (to) {
    *to += *from;
  } else {
    to = from;
  }
}

void Merge(SystemStats& to, const SystemStats& from) {
  boost::pfr::for_each_field(from, [&to](auto from_field, auto index) {
    MergeSingleStat(boost::pfr::get<decltype(index){}>(to), from_field);
  });
}

bool IsProcStatMatchesName(std::string_view data, std::string_view name) {
  size_t pos = data.find(' ');
  return pos != std::string_view::npos && pos + 2 + name.size() < data.size() &&
         data[pos + 1] == '(' && data.substr(pos + 2, name.size()) == name &&
         data[pos + 2 + name.size()] == ')';
}

bool IsAllDigits(const boost::filesystem::path& path) {
  return std::all_of(path.native().begin(), path.native().end(),
                     [](char c) { return c >= '0' && c <= '9'; });
}

void ParseProcStat(std::string_view data, SystemStats& stats) {
  static const auto kTicksPerSecond = sysconf(_SC_CLK_TCK);
  static const auto kPageSizeKb = sysconf(_SC_PAGESIZE) / 1024;

  // proc(5), 1-based
  static constexpr size_t kCommFieldNum = 2;
  static constexpr size_t kMajfltFieldNum = 12;
  static constexpr size_t kUtimeFieldNum = 14;
  static constexpr size_t kStimeFieldNum = 15;
  static constexpr size_t kRssFieldNum = 24;

  size_t pos = 0;
  size_t field_num = 0;
  int64_t utime_ticks = -1;
  while (pos < data.size()) {
    ++field_num;
    size_t next_delim_pos = std::min(data.find(' ', pos), data.size());

    const auto get_current_value = [=] {
      return utils::FromString<int64_t>(
          std::string{data.substr(pos, next_delim_pos - pos)});
    };

    switch (field_num) {
      case kCommFieldNum:
        // special case, as comm may contain spaces
        if (data[pos] != '(') {
          LOG_LIMITED_DEBUG() << "Unexpected comm format in '" << data << '\'';
          break;
        }
        next_delim_pos = data.find(')', pos) + 1;
        UASSERT(next_delim_pos < data.size() && data[next_delim_pos] == ' ');
        break;

      case kMajfltFieldNum:
        stats.major_pagefaults = get_current_value();
        break;

      case kUtimeFieldNum:
        utime_ticks = get_current_value();
        break;
      case kStimeFieldNum:
        UASSERT(utime_ticks != -1);
        {
          const auto total_ticks = utime_ticks + get_current_value();
          stats.cpu_time_sec =
              int64_t{total_ticks / kTicksPerSecond} +
              static_cast<double>(total_ticks % kTicksPerSecond) /
                  kTicksPerSecond;
        }
        break;

      case kRssFieldNum:
        stats.rss_kb = get_current_value() * kPageSizeKb;
        break;

      default:;
        // skip
    }

    pos = next_delim_pos + 1;
  }
}

void ParseProcStatIo(std::string_view data, SystemStats& stats) {
  static constexpr std::string_view kReadBytesHeader = "read_bytes: ";
  static constexpr std::string_view kWriteBytesHeader = "write_bytes: ";

  size_t pos = 0;
  while (pos < data.size()) {
    auto next_newline_pos = std::min(data.find('\n', pos), data.size());

    const auto parse_if_matches = [=](std::optional<std::int64_t>& field,
                                      std::string_view header) {
      if (data.substr(pos, header.size()) != header) return;

      auto value_pos = pos + header.size();
      UASSERT(value_pos < next_newline_pos);
      auto value_len = next_newline_pos - value_pos;
      field = utils::FromString<int64_t>(
          std::string{data.substr(value_pos, value_len)});
    };

    parse_if_matches(stats.io_read_bytes, kReadBytesHeader);
    parse_if_matches(stats.io_write_bytes, kWriteBytesHeader);

    pos = next_newline_pos + 1;
  }
}

SystemStats GetSystemStatisticsByProcPath(std::string_view path) {
  SystemStats stats;
  try {
    ParseProcStat(fs::blocking::ReadFileContents(fmt::format("{}/stat", path)),
                  stats);
  } catch (const std::exception& ex) {
    LOG_LIMITED_DEBUG() << "Could not get stats from " << path << ": " << ex;
  }

  {
    boost::system::error_code ec;
    boost::filesystem::directory_iterator it{fmt::format("{}/fd", path), ec};
    if (!ec) {
      stats.open_files = 0;
      while (!ec && it != boost::filesystem::directory_iterator{}) {
        ++*stats.open_files;
        it.increment(ec);
      }
    }
  }

  try {
    ParseProcStatIo(fs::blocking::ReadFileContents(fmt::format("{}/io", path)),
                    stats);
  } catch (const std::exception& ex) {
    LOG_LIMITED_DEBUG() << "Could not get I/O stats from " << path << ": "
                        << ex;
  }

  return stats;
}

SystemStats GetSystemStatisticsByExeNameFromProc(std::string_view name) {
  boost::system::error_code ec;
  boost::filesystem::directory_iterator it("/proc", ec);
  if (ec) {
    LOG_LIMITED_DEBUG() << "Could not collect stats for " << name << ": "
                        << ec.message();
    return {};
  }

  SystemStats cumulative;
  for (; !ec && it != boost::filesystem::directory_iterator{};
       it.increment(ec)) {
    if (!boost::filesystem::is_directory(it->status())) continue;
    if (!IsAllDigits(it->path().filename())) continue;

    std::string stat_data;
    try {
      stat_data =
          fs::blocking::ReadFileContents((it->path() / "stat").native());
    } catch (const std::exception&) {
      // most likely not a process directory or insufficient permissions
      continue;
    }
    auto exe_symlink = it->path() / "exe";

    if (IsProcStatMatchesName(stat_data, name) ||
        boost::filesystem::read_symlink(exe_symlink, ec).filename().native() ==
            name) {
      Merge(cumulative, GetSystemStatisticsByProcPath(it->path().native()));
    }
  }
  return cumulative;
}
#endif

#ifdef __APPLE__
utils::statistics::impl::SystemStats GetSelfSystemStatisticsFromKernel() {
  utils::statistics::impl::SystemStats stats;

  {
    struct mach_task_basic_info basic_info {};
    mach_msg_type_number_t info_count = MACH_TASK_BASIC_INFO_COUNT;
    if (::task_info(mach_task_self(), MACH_TASK_BASIC_INFO,
                    reinterpret_cast<task_info_t>(&basic_info),
                    &info_count) == KERN_SUCCESS) {
      stats.rss_kb = basic_info.resident_size / 1024;
    }
  }

  {
    boost::system::error_code ec;
    boost::filesystem::directory_iterator it{"/dev/fd", ec};
    if (!ec) {
      stats.open_files = 0;
      while (!ec && it != boost::filesystem::directory_iterator{}) {
        ++*stats.open_files;
        it.increment(ec);
      }
    }
  }

  {
    struct rusage rusage {};
    if (::getrusage(RUSAGE_SELF, &rusage) != -1) {
      timeradd(&rusage.ru_utime, &rusage.ru_stime, &rusage.ru_utime);
      stats.cpu_time_sec =
          rusage.ru_utime.tv_sec + rusage.ru_utime.tv_usec * 1e-6;
      stats.major_pagefaults = rusage.ru_majflt;
    }
  }

  return stats;
}
#endif

}  // namespace

void DumpMetric(Writer& writer, const SystemStats& stats) {
  const auto put_field = [&writer](std::string_view name, const auto& value) {
    if (value) writer[name] = *value;
  };
  put_field("cpu_time_sec", stats.cpu_time_sec);
  put_field("rss_kb", stats.rss_kb);
  put_field("open_files", stats.open_files);
  put_field("major_pagefaults", stats.major_pagefaults);
  put_field("io_read_bytes", stats.io_read_bytes);
  put_field("io_write_bytes", stats.io_write_bytes);
}

static_assert(kHasWriterSupport<SystemStats>);

SystemStats GetSelfSystemStatistics() {
#if defined(__linux__)
  return GetSystemStatisticsByProcPath("/proc/self");
#elif defined(__APPLE__)
  return GetSelfSystemStatisticsFromKernel();
#endif

  LOG_LIMITED_DEBUG() << "Skipped self statistics collection";
  return {};
}

SystemStats GetSystemStatisticsByExeName(std::string_view name) {
#if defined(__linux__)
  return GetSystemStatisticsByExeNameFromProc("nginx");
#endif

  LOG_LIMITED_DEBUG() << "Skipped " << name << " statistics collection";
  return {};
}

}  // namespace utils::statistics::impl

USERVER_NAMESPACE_END
