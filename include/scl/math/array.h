/* SCL --- Secure Computation Library
 * Copyright (C) 2024 Anders Dalskov
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef SCL_MATH_ARRAY_H
#define SCL_MATH_ARRAY_H

#include <array>
#include <cstddef>
#include <sstream>
#include <type_traits>

#include "scl/serialization/serializer.h"
#include "scl/util/prg.h"

namespace scl {
namespace math {

/// @cond

template <typename T>
concept PrefixIncrementable = requires(T a) { ++a; };

template <typename T>
concept PrefixDecrementable = requires(T a) { --a; };

template <typename T, typename V>
concept Multipliable = requires(T a, V b) { (a) * (b); };

template <typename T>
concept Divisble = requires(T a, T b) { (a) / (b); };

template <typename T, typename V>
struct MultiplyResultTypeTrait {
  using Type = decltype(std::declval<T>() * std::declval<V>());
};

template <typename T, typename V>
using MultiplyResultType = typename MultiplyResultTypeTrait<T, V>::Type;

template <typename T>
concept Invertible = requires(T& a) { a.invert(); };

/// @endcond

/**
 * @brief Array of values, e.g., group elements.
 * @tparam T the array element type.
 * @tparam N the number of elements.
 *
 * Array is effectively a wrapper around <code>std::array<T, N></code> with
 * added functionality that allows operating on Array objects as if they where
 * group or ring elements. As such, Array behaves sort of like a direct product
 * of N copies of the same group.
 */
template <typename T, std::size_t N>
class Array final {
 public:
  /**
   * @brief Binary size of this Array.
   */
  constexpr static std::size_t byteSize() {
    return T::byteSize() * N;
  }

  /**
   * @brief Read an array from a buffer.
   */
  static Array<T, N> read(const unsigned char* src) {
    Array<T, N> p;
    for (std::size_t i = 0; i < N; ++i) {
      p.m_values[i] = T::read(src + i * T::byteSize());
    }
    return p;
  }  // LCOV_EXCL_LINE

  /**
   * @brief Create an array filled with random elements.
   */
  static Array<T, N> random(util::PRG& prg)
    requires requires() { T::random(prg); }
  {
    Array<T, N> p;
    for (std::size_t i = 0; i < N; ++i) {
      p.m_values[i] = T::random(prg);
    }
    return p;
  }  // LCOV_EXCL_LINE

  /**
   * @brief Get an Array filled with the multiplicative identity
   */
  static Array<T, N> one()
    requires requires() { T::one(); }
  {
    return Array<T, N>(T::one());
  }

  /**
   * @brief Get an Array filled with the additive identity.
   */
  static Array<T, N> zero() {
    return Array<T, N>(T::zero());
  }

  /**
   * @brief Default constructor.
   */
  Array() {}

  /**
   * @brief Construct an Array filled with copies of the same element.
   * @param element the element.
   */
  Array(const T& element) {
    m_values.fill(element);
  }

  /**
   * @brief Construct an Array filled with copies of the same element.
   *
   * This function will attempt to construct a \p T element using \p value, and
   * then fill all slots with this value.
   */
  explicit Array(int value) : Array(T{value}){};

  /**
   * @brief Copy construct an Array from another array.
   * @param arr the array.
   */
  Array(const std::array<T, N>& arr) : m_values{arr} {}

  /**
   * @brief Move construct an Array from another array.
   * @param arr the array.
   */
  Array(std::array<T, N>&& arr) : m_values{std::move(arr)} {}

  /**
   * @brief Add another Array to this Array.
   */
  Array& operator+=(const Array& other) {
    for (std::size_t i = 0; i < N; ++i) {
      m_values[i] += other.m_values[i];
    }
    return *this;
  }

  /**
   * @brief Add two arrays.
   */
  friend Array operator+(const Array& lhs, const Array& rhs) {
    Array tmp(lhs);
    return tmp += rhs;
  }

  /**
   * @brief Prefix increment operator.
   */
  Array& operator++()
    requires PrefixIncrementable<T>
  {
    for (auto& v : m_values) {
      ++v;
    }
    return *this;
  }

  /**
   * @brief Postfix increment operator.
   */
  friend Array operator++(Array& arr, int)
    requires PrefixIncrementable<T>
  {
    Array tmp(arr);
    for (auto& v : arr.m_values) {
      ++v;
    }
    return tmp;
  }

  /**
   * @brief Subtract an Array from this.
   */
  Array& operator-=(const Array& other) {
    for (std::size_t i = 0; i < N; ++i) {
      m_values[i] -= other.m_values[i];
    }
    return *this;
  }

  /**
   * @brief Subtract two Arrays.
   */
  friend Array operator-(const Array& lhs, const Array& rhs) {
    Array tmp(lhs);
    return tmp -= rhs;
  }

  /**
   * @brief Prefix decrement operator.
   */
  Array& operator--()
    requires PrefixDecrementable<T>
  {
    for (auto& v : m_values) {
      --v;
    }
    return *this;
  }

  /**
   * @brief Postfix decrement operator.
   */
  friend Array operator--(Array& arr, int)
    requires PrefixDecrementable<T>
  {
    Array tmp(arr);
    for (auto& v : arr.m_values) {
      --v;
    }
    return tmp;
  }

  /**
   * @brief Negate this Array.
   */
  Array& negate() {
    for (std::size_t i = 0; i < N; ++i) {
      m_values[i].Negate();
    }
    return *this;
  }

  /**
   * @brief Multiply this Array with a scalar.
   */
  template <typename S>
  Array<T, N>& operator*=(const S& scalar)
    requires Multipliable<T, S>
  {
    for (std::size_t i = 0; i < N; i++) {
      m_values[i] *= scalar;
    }
    return *this;
  }

  /**
   * @brief Multiply two Arrays entry-wise.
   */
  template <typename S>
  Array<T, N>& operator*=(const Array<S, N>& other)
    requires Multipliable<T, S>
  {
    for (std::size_t i = 0; i < N; ++i) {
      m_values[i] *= other[i];
    }
    return *this;
  }

  /**
   * @brief Multiply a scalar with this Array.
   */
  template <typename S>
  Array<T, N> operator*(const S& scalar) const
    requires Multipliable<T, S>
  {
    auto copy = *this;
    return copy *= scalar;
  }

  /**
   * @brief Multiply two Arrays entry-wise.
   */
  template <typename V>
  friend Array<MultiplyResultType<T, V>, N> operator*(const Array<T, N>& lhs,
                                                      const Array<V, N>& rhs)
    requires Multipliable<T, V>
  {
    Array<MultiplyResultType<T, V>, N> tmp;
    for (std::size_t i = 0; i < N; i++) {
      tmp[i] = lhs[i] * rhs[i];
    }
    return tmp;
  }

  /**
   * @brief Invert all entries in this Array.
   */
  Array<T, N>& invert()
    requires Invertible<T>
  {
    for (std::size_t i = 0; i < N; ++i) {
      m_values[i].invert();
    }
    return *this;
  }

  /**
   * @brief Compute the inverse of each entry in this Array.
   */
  Array<T, N> Inverse() const
    requires Invertible<T>
  {
    Array<T, N> p = *this;
    return p.Invert();
  }

  /**
   * @brief Divide this Array by another Array entry-wise.
   */
  Array<T, N> operator/=(const Array<T, N>& other)
    requires Divisble<T>
  {
    for (std::size_t i = 0; i < N; ++i) {
      m_values[i] /= other[i];
    }
    return *this;
  }

  /**
   * @brief Divide this Array by another Array entry-wise.
   */
  Array<T, N> operator/(const Array<T, N>& other) const
    requires Divisble<T>
  {
    Array<T, N> r = *this;
    r /= other;
    return r;
  }  // LCOV_EXCL_LINE

  /**
   * @brief Get the value at a particular entry in the product element.
   */
  T& operator[](std::size_t index) {
    return m_values[index];
  }

  /**
   * @brief Get the value at a particular entry in the product element.
   */
  T operator[](std::size_t index) const {
    return m_values[index];
  }

  /**
   * @brief Compare two product elements.
   */
  bool equal(const Array& other) const {
    bool eq = true;
    for (std::size_t i = 0; i < N; ++i) {
      eq = eq && (m_values[i] == other.m_values[i]);
    }
    return eq;
  }

  /**
   * @brief Equality operator for Array.
   */
  friend bool operator==(const Array& lhs, const Array& rhs) {
    return lhs.equal(rhs);
  }

  /**
   * @brief In-equality operator for Array.
   */
  friend bool operator!=(const Array& lhs, const Array& rhs) {
    return !(lhs == rhs);
  }

  /**
   * @brief Get a string representation of this product element.
   */
  std::string toString() const {
    std::stringstream ss;
    ss << "P{";
    for (std::size_t i = 0; i < N - 1; ++i) {
      ss << m_values[i] << ", ";
    }
    ss << m_values[N - 1] << "}";
    return ss.str();
  }

  /**
   * @brief Stream printing operator for Array.
   */
  friend std::ostream& operator<<(std::ostream& os, const Array& array) {
    return os << array.toString();
  }

  /**
   * @brief Write this product element to a buffer.
   */
  void write(unsigned char* dest) const {
    for (std::size_t i = 0; i < N; ++i) {
      m_values[i].write(dest + i * T::byteSize());
    }
  }

 private:
  std::array<T, N> m_values;
};

}  // namespace math

namespace seri {

/**
 * @brief Serializer specialization for product types.
 */
template <typename GROUP, std::size_t N>
struct Serializer<math::Array<GROUP, N>> {
  /**
   * @brief Get the serialized size of a product type.
   */
  static constexpr std::size_t sizeOf(
      const math::Array<GROUP, N>& /* ignored */) {
    return math::Array<GROUP, N>::byteSize();
  }

  /**
   * @brief Write a product element to a buffer.
   * @param prod the element.
   * @param buf the buffer.
   */
  static std::size_t write(const math::Array<GROUP, N>& prod,
                           unsigned char* buf) {
    prod.write(buf);
    return sizeOf(prod);
  }

  /**
   * @brief Read a product element from a buffer.
   * @param prod the output.
   * @param buf the buffer.
   */
  static std::size_t read(math::Array<GROUP, N>& prod,
                          const unsigned char* buf) {
    prod = math::Array<GROUP, N>::read(buf);
    return sizeOf(prod);
  }
};

}  // namespace seri

}  // namespace scl

#endif  // SCL_MATH_ARRAY_H
