/**
 * This header defines some platform specific stuff if it is missing
 */

#ifndef COMPAT_H
#define COMPAT_H

#include <cstdint>

#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)

#if ((GCC_VERSION >= 40700) && (GCC_VERSION < 40800))
#warning "Self defined __builtin_bswap16"
static inline uint16_t __builtin_bswap16(uint16_t val)
{
  return (val << 8) | (val >> 8);
}
#endif

#endif
