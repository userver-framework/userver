#include <userver/error_injection/hook.hpp>

#include <thread>

#include <userver/utils/rand.hpp>

USERVER_NAMESPACE_BEGIN

namespace error_injection {

Verdict Hook::ReturnVerdict(const Settings& settings) {
    auto random = utils::RandRange(0.0, 1.0);
    if (random > settings.probability) {
        return Verdict::kSkip;
    }

    if (settings.possible_verdicts.empty()) {
        return Verdict::kError;
    }

    auto verdict_num = utils::RandRange(settings.possible_verdicts.size());
    return settings.possible_verdicts[verdict_num];
}

Hook::Hook(const Settings& settings, engine::Deadline deadline)
    : verdict_(settings.enabled ? ReturnVerdict(settings) : Verdict::kSkip),
      deadline_(deadline)
{}

engine::Deadline Hook::CalcPostHookDeadline() {
    constexpr auto kPassed = engine::Deadline::Passed();

    switch (verdict_) {
        case Verdict::kMaxDelay:
            return deadline_;

        case Verdict::kRandomDelay: {
            auto left = deadline_.TimeLeft().count();
            if (left < 0) {
                return kPassed;
            }

            auto delay = utils::RandRange(left + 1);
            return engine::Deadline::FromDuration(engine::Deadline::Clock::duration(delay));
        }

        case Verdict::kError:
        case Verdict::kSkip:
        case Verdict::kTimeout:
            return kPassed;
    }

    return kPassed;
}

}  // namespace error_injection

USERVER_NAMESPACE_END
