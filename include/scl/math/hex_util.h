#ifndef SCL_MATH_HEX_UTIL_H
#define SCL_MATH_HEX_UTIL_H

#include <cstddef>
#include <stdexcept>

namespace scl::math {

#define SCL_TO_HEX(v, c)                                                \
  do {                                                                  \
    if ((c) >= '0' && (c) <= '9')                                       \
      (v) += (c) - '0';                                                 \
    else if ((c) >= 'a' && (c) <= 'f')                                  \
      (v) += (c) - 'a' + 10;                                            \
    else if ((c) >= 'A' && (c) <= 'F')                                  \
      (v) += (c) - 'A' + 10;                                            \
    else                                                                \
      throw std::invalid_argument("encountered invalid hex character"); \
  } while (0)

/**
 * @brief Convert a string in hex.
 * @tparam T the output type
 *
 * Normal conventions for hex strings apply, although the <code>0x</code> prefix
 * is not permitted. The input is assumed to encode an integer in big endian.
 */
template <typename T>
T fromHexString(const std::string& s) {
  auto n = s.size();
  if (n % 2) {
    throw std::invalid_argument("odd-length hex string");
  }
  T t = 0;
  for (std::size_t i = 0; i < n; i += 2) {
    char c0 = s[i];
    char c1 = s[i + 1];
    t = t << 4;
    SCL_TO_HEX(t, c0);
    t = t << 4;
    SCL_TO_HEX(t, c1);
  }
  return t;
}

#undef SCL_TO_HEX

}  // namespace scl::math

#endif  // SCL_MATH_HEX_UTIL_H
