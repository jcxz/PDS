/*
 * Copyright (C) 2014 Matus Fedorko <xfedor01@stud.fit.vutbr.cz>
 *
 * This file is part of PDS.
 *
 * PDS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PDS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with PDS. If not, see <http://www.gnu.org/licenses/>.
 */

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
