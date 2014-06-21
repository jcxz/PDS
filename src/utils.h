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
