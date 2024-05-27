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

#ifndef SCL_MATH_VECTOR_H
#define SCL_MATH_VECTOR_H

#include <cstdint>
#include <cstring>
#include <functional>
#include <sstream>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include "scl/serialization/serializer.h"
#include "scl/util/prg.h"

namespace scl {
namespace math {

template <typename ELEMENT>
class Matrix;

/**
 * @brief Computes an inner product between two iterators.
 * @param xb start of the first iterator.
 * @param xe end of the first iterator.
 * @param yb start of the second iterator.
 * @return the inner product.
 */
template <typename T, typename IT0, typename IT1>
T innerProd(IT0 xb, IT0 xe, IT1 yb) {
  T v;
  while (xb != xe) {
    v += *xb++ * *yb++;
  }
  return v;
}  // LCOV_EXCL_LINE

/**
 * @brief Vector.
 *
 * This class is a thin wrapper around std::vector meant only to provide some
 * functionality that makes it behave like other classes present in SCUtil.
 */
template <typename ELEMENT>
class Vector final {
 public:
  friend struct seri::Serializer<Vector<ELEMENT>>;

  /**
   * @brief The type of vector elements.
   */
  using ValueType = ELEMENT;

  /**
   * @brief The type of a vector size.
   */
  using SizeType = std::uint32_t;

  /**
   * @brief Iterator type.
   */
  using iterator = typename std::vector<ELEMENT>::iterator;

  /**
   * @brief Const iterator type.
   */
  using const_iterator = typename std::vector<ELEMENT>::const_iterator;

  /**
   * @brief Reverse iterator type.
   */
  using reverse_iterator = typename std::vector<ELEMENT>::reverse_iterator;

  /**
   * @brief Reverse const iterator type.
   */
  using const_reverse_iterator =
      typename std::vector<ELEMENT>::const_reverse_iterator;

  /**
   * @brief Create a Vec and populate it with random elements.
   * @param n the size of the vector
   * @param prg a PRG used to generate random elements
   * @return a Vec with random elements.
   */
  static Vector<ELEMENT> random(std::size_t n, util::PRG& prg);

  /**
   * @brief Create a vector with values in a range.
   * @param start the start value, inclusive
   * @param end the end value, exclusive
   * @return a vector with values <code>[start, start + 1, ..., end - 1]</code>.
   */
  static Vector<ELEMENT> range(std::size_t start, std::size_t end);

  /**
   * @brief Create a vector with values in a range.
   * @param end the end value, exclusive.
   * @return a vector with values <code>[0, ..., end - 1]</code>.
   */
  static Vector<ELEMENT> range(std::size_t end) {
    return range(0, end);
  }

  /**
   * @brief Default constructor that creates an empty Vec.
   */
  Vector() {}

  /**
   * @brief Construct a new Vec of explicit size.
   * @param n the size
   */
  explicit Vector(std::size_t n) : m_values(n) {}

  /**
   * @brief Construct a vector from an initializer_list.
   * @param values an initializer_list
   */
  Vector(std::initializer_list<ELEMENT> values) : m_values(values) {}

  /**
   * @brief Construct a vector from an STL vector.
   * @param values an STL vector
   */
  Vector(const std::vector<ELEMENT>& values) : m_values(values) {}

  /**
   * @brief Move construct a vector from an STL vector.
   * @param values an STL vector
   */
  Vector(std::vector<ELEMENT>&& values) : m_values(std::move(values)) {}

  /**
   * @brief Construct a Vec from a pair of iterators.
   * @param first iterator pointing to the first element
   * @param last iterator pointing to the one past last element
   * @tparam It iterator type
   */
  template <typename IT>
  explicit Vector(IT first, IT last) : m_values(first, last) {}

  /**
   * @brief The size of the Vec.
   */
  SizeType size() const {
    return m_values.size();
  }

  /**
   * @brief Check if this Vec is empty.
   */
  bool empty() const {
    return size() == 0;
  }

  /**
   * @brief Mutable access to vector elements.
   */
  ELEMENT& operator[](std::size_t idx) {
    return m_values[idx];
  }

  /**
   * @brief Read only access to vector elements.
   */
  ELEMENT operator[](std::size_t idx) const {
    return m_values[idx];
  }

  /**
   * @brief Add two Vec objects entry-wise.
   * @param other the other vector
   * @return the sum of this and \p other.
   */
  Vector add(const Vector& other) const;

  /**
   * @brief Add two Vec objects entry-wise in-place.
   * @param other the other vector
   * @return the sum of this and \p other, assigned to this.
   */
  Vector& addInPlace(const Vector& other) {
    ensureCompatible(other);
    for (std::size_t i = 0; i < size(); i++) {
      m_values[i] += other.m_values[i];
    }
    return *this;
  }

  /**
   * @brief Subtract two Vec objects entry-wise.
   * @param other the other vector
   * @return the difference of this and \p other.
   */
  Vector subtract(const Vector& other) const;

  /**
   * @brief Subtract two Vec objects entry-wise in-place.
   * @param other the other vector
   * @return the difference of this and \p other, assigned to this.
   */
  Vector& subtractInPlace(const Vector& other) {
    ensureCompatible(other);
    for (std::size_t i = 0; i < size(); i++) {
      m_values[i] -= other.m_values[i];
    }
    return *this;
  }

  /**
   * @brief Multiply two Vec objects entry-wise.
   * @param other the other vector
   * @return the product of this and \p other.
   */
  Vector multiplyEntryWise(const Vector& other) const;

  /**
   * @brief Multiply two Vec objects entry-wise in-place.
   * @param other the other vector
   * @return the product of this and \p other, assigned to this.
   */
  Vector& multiplyEntryWiseInPlace(const Vector& other) {
    ensureCompatible(other);
    for (std::size_t i = 0; i < size(); i++) {
      m_values[i] *= other.m_values[i];
    }
    return *this;
  }

  /**
   * @brief Compute a dot product between this and another vector.
   * @param other the other vector
   * @return the dot (or inner) product of this and \p other.
   */
  ELEMENT dot(const Vector& other) const {
    ensureCompatible(other);
    return innerProd<ELEMENT>(begin(), end(), other.begin());
  }

  /**
   * @brief Compute the sum over entries of this vector.
   * @return the sum of the entries of this vector.
   */
  ELEMENT sum() const {
    ELEMENT sum;
    for (const auto& v : m_values) {
      sum += v;
    }
    return sum;
  }  // LCOV_EXCL_LINE

  /**
   * @brief Scale this vector by a constant.
   * @param scalar the scalar
   * @return a scaled version of this vector.
   */
  template <typename SCALAR>
    requires requires(const ELEMENT& e, const SCALAR& s) {
               { (e) * (s) } -> std::convertible_to<ELEMENT>;
             }
  Vector scalarMultiply(const SCALAR& scalar) const {
    std::vector<ELEMENT> r;
    r.reserve(size());
    for (const auto& v : m_values) {
      r.emplace_back(scalar * v);
    }
    return Vector(r);
  }

  /**
   * @brief Scale this vector in-place by a constant.
   * @param scalar the scalar
   * @return a scaled version of this vector.
   */
  template <typename SCALAR>
    requires requires(ELEMENT& e, const SCALAR& s) {
               { e *= s } -> std::convertible_to<ELEMENT>;
             }
  Vector& scalarMultiplyInPlace(const SCALAR& scalar) {
    for (auto& v : m_values) {
      v *= scalar;
    }
    return *this;
  }

  /**
   * @brief Test if this vector is equal to another vector in constant time.
   * @param other the other vector
   * @return true if this vector is equal to \p other and false otherwise.
   */
  bool equals(const Vector& other) const;

  /**
   * @brief Operator == overload for Vec.
   */
  friend bool operator==(const Vector& left, const Vector& right) {
    return left.equals(right);
  }

  /**
   * @brief Operator != overload for Vec.
   */
  friend bool operator!=(const Vector& left, const Vector& right) {
    return !(left == right);
  }

  /**
   * @brief Convert this vector into a 1-by-N row matrix.
   */
  Matrix<ELEMENT> toRowMatrix() const {
    return Matrix<ELEMENT>{1, size(), m_values};
  }

  /**
   * @brief Convert this vector into a N-by-1 column matrix.
   */
  Matrix<ELEMENT> toColumnMatrix() const {
    return Matrix<ELEMENT>{size(), 1, m_values};
  }

  /**
   * @brief Convert this Vec object to an std::vector.
   */
  std::vector<ELEMENT>& toStlVector() {
    return m_values;
  }

  /**
   * @brief Convert this Vec object to a const std::vector.
   */
  const std::vector<ELEMENT>& toStlVector() const {
    return m_values;
  }

  /**
   * @brief Extract a sub-vector
   * @param start the start index, inclusive
   * @param end the end index, exclusive
   * @return a sub-vector.
   */
  Vector<ELEMENT> subVector(std::size_t start, std::size_t end) const {
    if (start > end) {
      throw std::logic_error("invalid range");
    }
    return Vector<ELEMENT>(begin() + start, begin() + end);
  }

  /**
   * @brief Extract a sub-vector.
   *
   * This method is equivalent to <code>subVector(0, end)</code>.
   *
   * @param end the end index, exclusive
   * @return a sub-vector.
   */
  Vector<ELEMENT> subVector(std::size_t end) const {
    return subVector(0, end);
  }

  /**
   * @brief Return a string representation of this vector.
   */
  std::string toString() const;

  /**
   * @brief Write a string representation of this vector to a stream.
   */
  friend std::ostream& operator<<(std::ostream& os, const Vector<ELEMENT>& v) {
    return os << v.toString();
  }

  /**
   * @brief Returns the number of bytes that Write writes.
   */
  std::size_t byteSize() const {
    return size() * ELEMENT::byteSize();
  }

  /**
   * @brief Return an iterator pointing to the start of this Vec.
   */
  iterator begin() {
    return m_values.begin();
  }

  /**
   * @brief Provides a const iterator to the start of this Vec.
   */
  const_iterator begin() const {
    return m_values.begin();
  }

  /**
   * @brief Provides a const iterator to the start of this Vec.
   */
  const_iterator cbegin() const {
    return m_values.cbegin();
  }

  /**
   * @brief Provides an iterator pointing to the end of this Vec.
   */
  iterator end() {
    return m_values.end();
  }

  /**
   * @brief Provides a const iterator pointing to the end of this Vec.
   */
  const_iterator end() const {
    return m_values.end();
  }

  /**
   * @brief Provides a const iterator pointing to the end of this Vec.
   */
  const_iterator cend() const {
    return m_values.cend();
  }

  /**
   * @brief Provides a reverse iterator pointing to the end of this Vec.
   */
  reverse_iterator rbegin() {
    return m_values.rbegin();
  }

  /**
   * @brief Provides a reverse const iterator pointing to the end of this Vec.
   */
  const_reverse_iterator rbegin() const {
    return m_values.rbegin();
  }

  /**
   * @brief Provides a reverse const iterator pointing to the end of this Vec.
   */
  const_reverse_iterator crbegin() const {
    return m_values.crbegin();
  }

  /**
   * @brief Provides a reverse iterator pointing to the start of this Vec.
   */
  reverse_iterator rend() {
    return m_values.rend();
  }

  /**
   * @brief Provides a reverse const iterator pointing to the start of this Vec.
   */
  const_reverse_iterator rend() const {
    return m_values.rend();
  }

  /**
   * @brief Provides a reverse const iterator pointing to the start of this Vec.
   */
  const_reverse_iterator crend() const {
    return m_values.crend();
  }

 private:
  void ensureCompatible(const Vector& other) const {
    if (size() != other.size()) {
      throw std::invalid_argument("Vec sizes mismatch");
    }
  }

  std::vector<ELEMENT> m_values;
};

template <typename ELEMENT>
Vector<ELEMENT> Vector<ELEMENT>::range(std::size_t start, std::size_t end) {
  if (start > end) {
    throw std::invalid_argument("invalid range");
  }
  if (start == end) {
    return Vector<ELEMENT>{};
  }

  std::vector<ELEMENT> v;
  v.reserve(end - start);
  for (std::size_t i = start; i < end; ++i) {
    v.emplace_back(ELEMENT{(int)i});
  }
  return Vector<ELEMENT>(v);
}

template <typename ELEMENT>
Vector<ELEMENT> Vector<ELEMENT>::random(std::size_t n, util::PRG& prg) {
  auto buf = std::make_unique<unsigned char[]>(n * ELEMENT::byteSize());
  prg.next(buf.get(), n * ELEMENT::byteSize());

  std::vector<ELEMENT> elements;
  elements.reserve(n);
  for (std::size_t i = 0; i < n; ++i) {
    elements.emplace_back(ELEMENT::read(buf.get() + i * ELEMENT::byteSize()));
  }

  return Vector<ELEMENT>(elements);
}

template <typename ELEMENT>
Vector<ELEMENT> Vector<ELEMENT>::add(const Vector<ELEMENT>& other) const {
  ensureCompatible(other);
  std::vector<ELEMENT> r;
  auto n = size();
  r.reserve(n);
  for (std::size_t i = 0; i < n; i++) {
    r.emplace_back(m_values[i] + other.m_values[i]);
  }
  return Vector(r);
}

template <typename ELEMENT>
Vector<ELEMENT> Vector<ELEMENT>::subtract(const Vector<ELEMENT>& other) const {
  ensureCompatible(other);
  std::vector<ELEMENT> r;
  auto n = size();
  r.reserve(n);
  for (std::size_t i = 0; i < n; i++) {
    r.emplace_back(m_values[i] - other.m_values[i]);
  }
  return Vector(r);
}

template <typename ELEMENT>
Vector<ELEMENT> Vector<ELEMENT>::multiplyEntryWise(
    const Vector<ELEMENT>& other) const {
  ensureCompatible(other);
  std::vector<ELEMENT> r;
  auto n = size();
  r.reserve(n);
  for (std::size_t i = 0; i < n; i++) {
    r.emplace_back(m_values[i] * other.m_values[i]);
  }
  return Vector(r);
}

template <typename ELEMENT>
bool Vector<ELEMENT>::equals(const Vector<ELEMENT>& other) const {
  if (size() != other.size()) {
    return false;
  }

  bool equal = true;
  for (std::size_t i = 0; i < size(); i++) {
    equal &= m_values[i] == other.m_values[i];
  }

  return equal;
}

template <typename ELEMENT>
std::string Vector<ELEMENT>::toString() const {
  if (empty()) {
    return "[ EMPTY VECTOR ]";
  }

  std::stringstream ss;
  ss << "[";
  std::size_t i = 0;
  for (; i < size() - 1; i++) {
    ss << m_values[i] << ", ";
  }
  ss << m_values[i] << "]";
  return ss.str();
}

}  // namespace math

namespace seri {  // namespace seri

/**
 * @brief Serializer specialization for math::Vec.
 */
template <typename ELEMENT>
struct Serializer<math::Vector<ELEMENT>> {
 private:
  using S_vec = Serializer<std::vector<ELEMENT>>;

 public:
  /**
   * @brief Size of a vector.
   * @param vec the vector.
   */
  static std::size_t sizeOf(const math::Vector<ELEMENT>& vec) {
    return S_vec::sizeOf(vec.m_values);
  }

  /**
   * @brief Write a math::Vec to a buffer.
   * @param vec the vector.
   * @param buf the buffer.
   */
  static std::size_t write(const math::Vector<ELEMENT>& vec,
                           unsigned char* buf) {
    return S_vec::write(vec.m_values, buf);
  }

  /**
   * @brief Read a math::Vec from a buf.
   * @param vec the vector.
   * @param buf the buffer.
   * @return the number of bytes read.
   */
  static std::size_t read(math::Vector<ELEMENT>& vec,
                          const unsigned char* buf) {
    return S_vec::read(vec.m_values, buf);
  }
};

}  // namespace seri
}  // namespace scl

#endif  // SCL_MATH_VECTOR_H
