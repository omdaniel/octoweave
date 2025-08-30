#include <cstdio>
namespace octoweave { namespace detail {
inline void log(const char* s){ std::fputs(s, stdout); std::fputc('\n', stdout); }
} }
