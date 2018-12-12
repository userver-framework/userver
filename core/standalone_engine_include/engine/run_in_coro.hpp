#include <functional>

void RunInCoro(std::function<void()> user_cb, size_t worker_threads);
