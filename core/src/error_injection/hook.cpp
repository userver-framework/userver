#include <error_injection/hook.hpp>

#include <random>
#include <thread>

namespace error_injection {

namespace {
std::mt19937& GetRng() {
  // Seed is not significant for error injection
  thread_local std::mt19937 gen(
      std::hash<std::thread::id>()(std::this_thread::get_id()));
  return gen;
}
}  // namespace

Verdict Hook::ReturnVerdict(const Settings& settings) {
  auto& gen = GetRng();

  auto random = std::uniform_real_distribution<>(0, 1)(gen);
  if (random > settings.probability) return Verdict::Skip;

  if (settings.possible_verdicts.empty()) return Verdict::Error;

  auto verdict_num = std::uniform_int_distribution<>(
      0, settings.possible_verdicts.size() - 1)(gen);
  return settings.possible_verdicts[verdict_num];
}

Hook::Hook(const Settings& settings, engine::Deadline deadline)
    : verdict_(settings.enabled ? ReturnVerdict(settings) : Verdict::Skip),
      deadline_(deadline) {}

engine::Deadline Hook::CalcPostHookDeadline() {
  static const auto kPassed =
      engine::Deadline::FromTimePoint(engine::Deadline::kPassed);
  switch (verdict_) {
    case Verdict::MaxDelay:
      return deadline_;

    case Verdict::RandomDelay: {
      auto left = deadline_.TimeLeft().count();
      if (left < 0) return kPassed;

      auto delay = std::uniform_int_distribution<>(0, left)(GetRng());
      return engine::Deadline::FromDuration(
          engine::Deadline::Clock::duration(delay));
    }

    case Verdict::Error:
    case Verdict::Skip:
    case Verdict::Timeout:
      return kPassed;
  }

  return kPassed;
}

}  // namespace error_injection
