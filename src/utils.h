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

#ifndef UTILS_H
#define UTILS_H

#include <type_traits>


namespace utils {

template<typename T, unsigned max_digits>
inline const char *toString(T num, typename std::enable_if<
                                     std::is_integral<T>::value &&
                                     std::is_unsigned<T>::value
                                   >::type * = nullptr)
{
  static char buffer[max_digits + 1] = { 0 };
  char *p_buf = buffer + max_digits;

  do {
    T next = num / 10;
    *--p_buf = "0123456789"[num % 10];
    num = next;
  } while (num > 0);

  return p_buf;
}

}

#endif
