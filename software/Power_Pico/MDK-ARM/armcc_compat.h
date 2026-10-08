#ifndef POWER_PICO_ARMCC_COMPAT_H
#define POWER_PICO_ARMCC_COMPAT_H

/* ARMCC 5 supports C99, but not the C11 static-assert keyword. */
#if defined(__CC_ARM) && !defined(__clang__)
#define POWER_PICO_ASSERT_JOIN_IMPL(a, b) a##b
#define POWER_PICO_ASSERT_JOIN(a, b) POWER_PICO_ASSERT_JOIN_IMPL(a, b)
#define _Static_assert(condition, message) \
    typedef char POWER_PICO_ASSERT_JOIN(power_pico_static_assert_, __LINE__)[(condition) ? 1 : -1]
#endif

#endif
