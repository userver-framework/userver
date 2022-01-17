#include <cstdlib>

#ifdef USERVER_FEATURE_REDIS_HI_MALLOC

// Workaround for https://bugs.launchpad.net/ubuntu/+source/hiredis/+bug/1888025
// Disable/enable it using cmake option USERVER_FEATURE_REDIS_HI_MALLOC
void* hi_malloc(size_t size) { return std::malloc(size); }

#endif
