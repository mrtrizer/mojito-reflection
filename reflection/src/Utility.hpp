#pragma once

#include <sstream>
#include <string>
#include <cassert>

namespace mojito {

using MojitoException = std::runtime_error;

// Use DEBUG_ASSERT only for next applications:
// 1. Check class has valid state.
// 2. Error can't be handled. Like memory is broken.
// Notice that DEBUG_ASSERT is turned off in production.
#ifdef NDEBUG
    #define DEBUG_ASSERT (void)
#else
// FIXME: Use std::terminate and custom output
    #define DEBUG_ASSERT(statement) assert(statement && __FILE__ && __LINE__)
    //#define DEBUG_ASSERT (void)
#endif

#ifdef NDEBUG
    #define FORDEBUG(expression)
#else
    #define FORDEBUG(expression) expression
#endif

template <typename... Args>
std::string concat(Args &&... args) noexcept
{
    std::ostringstream sstr;
    (sstr << ... << args);
    return sstr.str();
}

template <typename BaseT, typename DerivedT>
constexpr void assertDerived() {
    static_assert(std::is_base_of<BaseT, DerivedT>::value, "DerivedT should be derived from BaseT");
}

} // mojito
