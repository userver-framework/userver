#pragma once

#include <optional>
#include <string>
#include <vector>

#include <fmt/ranges.h>
#include <boost/container/small_vector.hpp>

#include <userver/logging/fwd.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/str_icase.hpp>
#include <userver/utils/strong_typedef.hpp>

#include <userver/storages/redis/command_options.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis::impl {

class CmdArgs;

class CmdWithArgs {
    friend class CmdArgs;

public:
    // channel is used for periodic subscribe/unsubscribe to calculate actual RTT
    // instead of sending PING commands which are not supported by hiredis in
    // subscriber mode
    static constexpr std::string_view kSubscriberPingChannelName = "_ping_dummy_ch";

    CmdWithArgs() = delete;
    CmdWithArgs(const CmdWithArgs&) = default;
    CmdWithArgs(CmdWithArgs&&) = default;
    CmdWithArgs& operator=(const CmdWithArgs&) = default;
    CmdWithArgs& operator=(CmdWithArgs&&) = default;

    template <typename... Args>
    CmdWithArgs(std::string&& command_name, Args&&... arguments) {
        UASSERT(!command_name.empty());
        UASSERT(command_name.size() == std::strlen(command_name.c_str()));
        args_.reserve(1 + sizeof...(Args));
        PutArg(std::move(command_name));
        (this->PutArg(std::forward<Args>(arguments)), ...);
    }

    const std::string& GetCommandName() const noexcept {
        UASSERT(!args_.empty());
        UASSERT(!args_[0].empty());
        return args_[0];
    }

    std::size_t GetCommandBytesLength() const {
        std::size_t size = 0;
        for (const auto& arg : args_) {
            size += arg.size();
        }
        UASSERT(size != 0);
        return size;
    }

    struct CommandChannel {
        const std::string& command;
        const std::string& channel;
    };

    CommandChannel GetCommandAndChannel() const noexcept {
        UASSERT(args_.size() == 2);
        return CommandChannel{args_[0], args_[1]};
    }

    bool IsMultiCommand() const noexcept {
        static constexpr std::string_view multi_command{"MULTI"};
        return utils::StrIcaseEqual{}(GetCommandName(), multi_command);
    }

    bool IsUnsubscribeCommand() const noexcept {
        static constexpr std::string_view unsubscribe_command{"UNSUBSCRIBE"};
        static constexpr std::string_view punsubscribe_command{"PUNSUBSCRIBE"};
        static constexpr std::string_view sunsubscribe_command{"SUNSUBSCRIBE"};

        return utils::StrIcaseEqual{}(GetCommandName(), unsubscribe_command) ||
               utils::StrIcaseEqual{}(GetCommandName(), punsubscribe_command) ||
               utils::StrIcaseEqual{}(GetCommandName(), sunsubscribe_command);
    }

    bool IsSubscribeCommand() const noexcept {
        static constexpr std::string_view subscribe_command{"SUBSCRIBE"};
        static constexpr std::string_view psubscribe_command{"PSUBSCRIBE"};
        static constexpr std::string_view ssubscribe_command{"SSUBSCRIBE"};

        return utils::StrIcaseEqual{}(GetCommandName(), subscribe_command) ||
               utils::StrIcaseEqual{}(GetCommandName(), psubscribe_command) ||
               utils::StrIcaseEqual{}(GetCommandName(), ssubscribe_command);
    }

    bool IsSubscribesCommand() const noexcept { return IsSubscribeCommand() || IsUnsubscribeCommand(); }

    bool IsExecCommand() const noexcept {
        static constexpr std::string_view exec_command{"EXEC"};
        return utils::StrIcaseEqual{}(GetCommandName(), exec_command);
    }

    bool IsSubscriberPingChannel() const noexcept { return args_.size() > 1 && args_[1] == kSubscriberPingChannelName; }

    template <class PointersStorage, class SizesStorage>
    void FillPointerSizesStorages(PointersStorage& pointers, SizesStorage& sizes) const {
        const auto size = args_.size();

        pointers.reserve(size);
        sizes.reserve(size);

        for (const auto& arg : args_) {
            pointers.push_back(arg.data());
            sizes.push_back(arg.size());
        }
    }

    auto GetJoinedArgs(const char* separator) const { return fmt::join(args_, separator); }

private:
    void PutArg(const char* arg);

    void PutArg(std::string_view arg);

    void PutArg(const std::string& arg);

    void PutArg(std::string&& arg);

    void PutArg(const std::vector<std::string>& arg);

    void PutArg(const std::vector<std::pair<std::string, std::string>>& arg);

    void PutArg(const std::vector<std::pair<double, std::string>>& arg);

    void PutArg(std::optional<ScanOptionsGeneric::Match> arg);
    void PutArg(std::optional<ScanOptionsGeneric::Count> arg);
    void PutArg(GeoaddArg arg);
    void PutArg(std::vector<GeoaddArg> arg);
    void PutArg(const GeoradiusOptions& arg);
    void PutArg(const GeosearchOptions& arg);
    void PutArg(const SetOptions& arg);
    void PutArg(const ZaddOptions& arg);
    void PutArg(const ScanOptions& arg);

    void PutArg(const ScoreOptions& arg);
    void PutArg(const RangeOptions& arg);
    void PutArg(const RangeScoreOptions& arg);

    template <typename Arg>
    typename std::enable_if<std::is_arithmetic<Arg>::value, void>::type PutArg(const Arg& arg) {
        args_.emplace_back(std::to_string(arg));
    }

    static constexpr std::size_t kTypicalArgsCount = 4;
    boost::container::small_vector<std::string, kTypicalArgsCount> args_;

    friend logging::LogHelper& operator<<(logging::LogHelper& os, const CmdWithArgs& v);
};

class CmdArgs {
public:
    template <typename... Args>
    CmdArgs(std::string command_name, Args&&... arguments) {
        commands_.emplace_back(std::move(command_name), std::forward<Args>(arguments)...);
    }

    CmdArgs(const CmdArgs&) = delete;
    CmdArgs(CmdArgs&&) = default;

    CmdArgs& operator=(const CmdArgs&) = delete;
    CmdArgs& operator=(CmdArgs&&) = default;

    template <typename... Args>
    CmdArgs& Then(std::string command_name, Args&&... arguments) {
        commands_.emplace_back(std::move(command_name), std::forward<Args>(arguments)...);
        return *this;
    }

    CmdArgs Clone() const {
        CmdArgs r;
        r.commands_ = commands_;
        return r;
    }

    std::size_t GetCommandCount() const noexcept { return commands_.size(); }

    const std::string& GetCommandName(std::size_t index) const noexcept {
        UASSERT(commands_.size() > index);
        return commands_[index].GetCommandName();
    }

    auto GetCommandAndChannel() const noexcept {
        UASSERT(commands_.size() == 1);
        return commands_[0].GetCommandAndChannel();
    }

    auto begin() const noexcept {
        UASSERT(!commands_.empty());
        return commands_.cbegin();
    }

    auto end() const noexcept {
        UASSERT(!commands_.empty());
        return commands_.cend();
    }

private:
    CmdArgs() = default;

    static constexpr std::size_t kTypicalCommandsCount = 1;
    boost::container::small_vector<CmdWithArgs, kTypicalCommandsCount> commands_;

    friend logging::LogHelper& operator<<(logging::LogHelper& os, const CmdArgs& v);
};

}  // namespace storages::redis::impl

USERVER_NAMESPACE_END
