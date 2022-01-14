#include <dump/dump_locator.hpp>

#include <algorithm>

#include <fmt/compile.h>
#include <fmt/format.h>
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>

#include <userver/fs/blocking/write.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/datetime.hpp>
#include <userver/utils/from_string.hpp>

USERVER_NAMESPACE_BEGIN

namespace dump {

namespace {

const std::string kTimeZone = "UTC";

}  // namespace

DumpLocator::DumpLocator()
    : filename_regex_(GenerateFilenameRegex(FileFormatType::kNormal)),
      tmp_filename_regex_(GenerateFilenameRegex(FileFormatType::kTmp)) {}

DumpFileStats DumpLocator::RegisterNewDump(TimePoint update_time,
                                           const Config& config) {
  (void)this;  // silence tidy
  std::string dump_path = GenerateDumpPath(update_time, config);

  if (boost::filesystem::exists(dump_path)) {
    throw std::runtime_error(fmt::format(
        "{}: could not dump to \"{}\", because the file already exists",
        config.name, dump_path));
  }

  try {
    fs::blocking::CreateDirectories(config.dump_directory);
  } catch (const std::exception& ex) {
    throw std::runtime_error(
        fmt::format("{}: error while creating dump at \"{}\". Cause: {}",
                    config.name, dump_path, ex.what()));
  }

  return {update_time, std::move(dump_path), config.dump_format_version};
}

std::optional<DumpFileStats> DumpLocator::GetLatestDump(
    const Config& config) const {
  try {
    std::optional<DumpFileStats> stats = GetLatestDumpImpl(config);
    if (!stats) {
      LOG_INFO() << config.name << ": no usable dumps found";
      return std::nullopt;
    }

    LOG_DEBUG() << config.name << ": a usable dump found, path=\""
                << stats->full_path << "\"";

    return *std::move(stats);
  } catch (const std::exception& ex) {
    LOG_ERROR() << config.name
                << ": error while trying to read the contents of dump. Cause: "
                << ex;
    return std::nullopt;
  }
}

bool DumpLocator::BumpDumpTime(TimePoint old_update_time,
                               TimePoint new_update_time,
                               const Config& config) {
  (void)this;  // silence tidy
  if (new_update_time < old_update_time) {
    LOG_WARNING() << config.name << ": new_update_time < old_update_time, new="
                  << utils::datetime::Timestring(new_update_time, kTimeZone,
                                                 kFilenameDateFormat)
                  << ", old="
                  << utils::datetime::Timestring(old_update_time, kTimeZone,
                                                 kFilenameDateFormat);
  }

  const std::string old_name = GenerateDumpPath({old_update_time}, config);
  const std::string new_name = GenerateDumpPath({new_update_time}, config);

  try {
    if (!boost::filesystem::is_regular_file(old_name)) {
      LOG_WARNING()
          << config.name << ": the previous dump \"" << old_name
          << "\" has suddenly disappeared. A new dump will be created.";
      return false;
    }
    boost::filesystem::rename(old_name, new_name);
    LOG_INFO() << config.name << ": renamed dump \"" << old_name << "\" to \""
               << new_name << "\"";
    return true;
  } catch (const boost::filesystem::filesystem_error& ex) {
    LOG_ERROR() << config.name << ": error while trying to rename dump \""
                << old_name << " to \"" << new_name << "\". Reason: " << ex;
    return false;
  }
}

void DumpLocator::Cleanup(const Config& config) {
  const auto min_update_time = MinAcceptableUpdateTime(config);
  std::vector<DumpFileStats> dumps;

  try {
    if (!boost::filesystem::exists(config.dump_directory)) {
      LOG_INFO() << "Dump directory \"" << config.dump_directory
                 << "\" does not exist";
      return;
    }

    for (const auto& file :
         boost::filesystem::directory_iterator{config.dump_directory}) {
      if (!boost::filesystem::is_regular_file(file.status())) {
        continue;
      }

      std::string filename = file.path().filename().string();

      if (boost::regex_match(filename, tmp_filename_regex_)) {
        LOG_DEBUG() << "Removing a leftover tmp file \"" << file.path().string()
                    << "\"";
        boost::filesystem::remove(file);
        continue;
      }

      auto dump = ParseDumpName(file.path().string());
      if (!dump) {
        LOG_WARNING() << config.name
                      << ": unrelated file in the dump directory, path=\""
                      << file.path().string() << "\"";
        continue;
      }

      if (dump->format_version < config.dump_format_version ||
          dump->update_time < min_update_time) {
        LOG_DEBUG() << config.name << ": removing an expired dump, path=\""
                    << file.path().string() << "\"";
        boost::filesystem::remove(file);
        continue;
      }

      if (dump->format_version == config.dump_format_version) {
        dumps.push_back(std::move(*dump));
      }
    }

    std::sort(dumps.begin(), dumps.end(),
              [](const DumpFileStats& a, const DumpFileStats& b) {
                return a.update_time > b.update_time;
              });

    for (size_t i = config.max_dump_count; i < dumps.size(); ++i) {
      LOG_DEBUG() << config.name << ": removing an excessive dump \""
                  << dumps[i].full_path << "\"";
      boost::filesystem::remove(dumps[i].full_path);
    }
  } catch (const std::exception& ex) {
    LOG_ERROR() << config.name
                << ": error while cleaning up old dumps. Cause: " << ex;
  }
}

std::optional<DumpFileStats> DumpLocator::ParseDumpName(
    std::string full_path) const {
  const auto filename = boost::filesystem::path{full_path}.filename().string();

  boost::smatch regex;
  if (boost::regex_match(filename, regex, filename_regex_)) {
    UASSERT_MSG(regex.size() == 3,
                fmt::format("Incorrect sub-match count: {} for filename {}",
                            regex.size(), filename));

    try {
      const auto date_string = regex[1].str();
      const auto date_format = date_string.find(':') == std::string::npos
                                   ? kFilenameDateFormat
                                   : kLegacyFilenameDateFormat;
      const auto date =
          utils::datetime::Stringtime(date_string, kTimeZone, date_format);
      const auto version = utils::FromString<uint64_t>(regex[2].str());
      return DumpFileStats{{Round(date)}, std::move(full_path), version};
    } catch (const std::exception& ex) {
      LOG_WARNING() << "A filename looks like a dump, but it is not, path=\""
                    << filename << "\". Reason: " << ex;
      return std::nullopt;
    }
  }
  return std::nullopt;
}

std::optional<DumpFileStats> DumpLocator::GetLatestDumpImpl(
    const Config& config) const {
  const auto min_update_time = MinAcceptableUpdateTime(config);
  std::optional<DumpFileStats> best_dump;

  try {
    if (!boost::filesystem::exists(config.dump_directory)) {
      LOG_DEBUG() << "Dump directory \"" << config.dump_directory
                  << "\" does not exist";
      return {};
    }

    for (const auto& file :
         boost::filesystem::directory_iterator{config.dump_directory}) {
      if (!boost::filesystem::is_regular_file(file.status())) {
        continue;
      }

      auto curr_dump = ParseDumpName(file.path().string());
      if (!curr_dump) {
        if (boost::regex_match(file.path().filename().string(),
                               tmp_filename_regex_)) {
          LOG_DEBUG() << "A leftover tmp file found: \"" << file.path().string()
                      << "\". It will be removed on next Cleanup";
        } else {
          LOG_WARNING() << "Unrelated file in the dump directory: \""
                        << file.path().string() << "\"";
        }
        continue;
      }

      if (curr_dump->format_version != config.dump_format_version) {
        LOG_DEBUG() << "Ignoring dump \"" << curr_dump->full_path
                    << "\", because its format version ("
                    << curr_dump->format_version << ") != current version ("
                    << config.dump_format_version << ")";
        continue;
      }

      if (curr_dump->update_time < min_update_time && config.max_dump_age) {
        LOG_DEBUG() << "Ignoring dump \"" << curr_dump->full_path
                    << "\", because its age is greater than the maximum "
                       "allowed dump age ("
                    << config.max_dump_age->count() << "ms)";
        continue;
      }

      if (!best_dump || curr_dump->update_time > best_dump->update_time) {
        best_dump = std::move(curr_dump);
      }
    }
  } catch (const std::exception& ex) {
    LOG_ERROR() << config.name
                << ": error while trying to fetch dumps. Cause: " << ex;
    // proceed to return best_dump
  }

  return best_dump ? std::optional{std::move(best_dump)} : std::nullopt;
}

std::string DumpLocator::GenerateDumpPath(TimePoint update_time,
                                          const Config& config) {
  return fmt::format(
      FMT_COMPILE("{}/{}-v{}"), config.dump_directory,
      utils::datetime::Timestring(update_time, kTimeZone, kFilenameDateFormat),
      config.dump_format_version);
}

std::string DumpLocator::GenerateFilenameRegex(FileFormatType type) {
  return std::string{
             R"(^(\d{4}-\d{2}-\d{2}T\d{2}:?\d{2}:?\d{2}\.\d{6}Z?)-v(\d+))"} +
         (type == FileFormatType::kTmp ? "\\.tmp$" : "$");
}

TimePoint DumpLocator::MinAcceptableUpdateTime(const Config& config) {
  return config.max_dump_age
             ? Round(utils::datetime::Now()) - *config.max_dump_age
             : TimePoint::min();
}

TimePoint DumpLocator::Round(std::chrono::system_clock::time_point time) {
  return std::chrono::round<TimePoint::duration>(time);
}

}  // namespace dump

USERVER_NAMESPACE_END
