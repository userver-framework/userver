#include "sink_helper_test.hpp"

#include <boost/locale.hpp>

#include <userver/fs/blocking/read.hpp>
#include <userver/utils/regex.hpp>
#include <userver/utils/text.hpp>

USERVER_NAMESPACE_BEGIN

namespace test {

const utils::regex regexp_pattern{R"(\[\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}.\d{3}\])"};
const std::string result_pattern{"[DATETIME]"};

std::vector<std::string> NormalizeLogs(const std::string& data) {
    const auto logs = utils::text::Split(data, "\n");
    std::vector<std::string> result;
    result.reserve(logs.size());

    const boost::locale::generator gen;
    const std::locale loc = gen("");
    for (const auto& log : logs) {
        auto replace_datetime = utils::regex_replace(log, regexp_pattern, result_pattern);
        if (!replace_datetime.empty()) {
            result.push_back(boost::locale::to_lower(replace_datetime, loc));
        }
    }
    return result;
}

std::vector<std::string> ReadFromFile(const std::string& filename) {
    return NormalizeLogs(fs::blocking::ReadFileContents(filename));
}

std::vector<std::string> ReadFromFd(fs::blocking::FileDescriptor&& fd) {
    char buf[2048];
    std::string data{};
    auto read_size = 0;
    do {
        read_size = fd.Read(buf, sizeof(buf));
        data.append(buf, read_size);
    } while (read_size > 0);
    return test::NormalizeLogs(data);
}

std::vector<std::string> ReadFromSocket(engine::io::Socket&& sock) {
    char buf[1024];
    std::string data{};
    auto read_size = 0;
    do {
        read_size = sock.ReadSome(buf, sizeof(buf), {});
        data.append(buf, read_size);
    } while (read_size > 0);
    return test::NormalizeLogs(data);
}

}  // namespace test

USERVER_NAMESPACE_END
