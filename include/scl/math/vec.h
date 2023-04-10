/* SCL --- Secure Computation Library
 * Copyright (C) 2023 Anders Dalskov
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

#ifndef SCL_MATH_VEC_H
#define SCL_MATH_VEC_H

#include <cstring>
#include <functional>
#include <sstream>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include "scl/util/prg.h"
#include "scl/util/traits.h"

namespace scl::math {

template <typename Elem>
class Mat;

/**
 * @brief Computes an unchecked inner product between two iterators.
 * @param xb start of the first iterator
 * @param xe end of the first iterator
 * @param yb start of the second iterator
 */
template <typename T, typename I0, typename I1>
T UncheckedInnerProd(I0 xb, I0 xe, I1 yb) {
  T v;
  while (xb != xe) {
    v += *xb++ * *yb++;
  }
  return v;
}

/**
 * @brief Vector.
 *
 * This class is a thin wrapper around std::vector meant only to provide some
 * functionality that makes it behave like other classes present in SCUtil.
 */
template <typename Elem>
class Vec {
  static_assert(util::Serializable<Elem>::value,
                "Vector elements must have Read/Write methods");

 public:
  /**
   * @brief The type of vector elements.
   */
  using ValueType = Elem;

  /**
   * @brief Iterator type.
   */
  using iterator = typename std::vector<Elem>::iterator;

  /**
   * @brief Const iterator type.
   */
  using const_iterator = typename std::vector<Elem>::const_iterator;

  /**
   * @brief Reverse iterator type.
   */
  using reverse_iterator = typename std::vector<Elem>::reverse_iterator;

  /**
   * @brief Reverse const iterator type.
   */
  using const_reverse_iterator =
      typename std::vector<Elem>::const_reverse_iterator;

  /**
   * @brief Read a vec from a stream of bytes.
   * @param n the number of elements to read
   * @param src the buffer to read from
   */
  static Vec<Elem> Read(std::size_t n, const unsigned char* src);

  /**
   * @brief Create a Vec and populate it with random elements.
   * @param n the size of the vector
   * @param prg a PRG used to generate random elements
   * @return a Vec with random elements.
   */
  static Vec<Elem> Random(std::size_t n, util::PRG& prg);

  /**
   * @brief Create a vector with values in a range.
   * @param start the start value, inclusive
   * @param end the end value, exclusive
   * @return a vector with values <code>[start, start + 1, ..., end - 1]</code>.
   */
  static Vec<Elem> Range(std::size_t start, std::size_t end);

  /**
   * @brief Create a vector with values in a range.
   * @param end the end value, exclusive.
   * @return a vector with values <code>[0, ..., end - 1]</code>.
   */
  static Vec<Elem> Range(std::size_t end) {
    return Range(0, end);
  }

  /**
   * @brief Default constructor that creates an empty Vec.
   */
  Vec(){};

  /**
   * @brief Construct a new Vec of explicit size.
   * @param n the size
   */
  explicit Vec(std::size_t n) : m_values(n){};

  /**
   * @brief Construct a vector from an initializer_list.
   * @param values an initializer_list
   */
  Vec(std::initializer_list<Elem> values) : m_values(values){};

  /**
   * @brief Construct a vector from an STL vector.
   * @param values an STL vector
   */
  Vec(const std::vector<Elem>& values) : m_values(values){};

  /**
   * @brief Move construct a vector from an STL vector.
   * @param values an STL vector
   */
  Vec(std::vector<Elem>&& values) : m_values(std::move(values)){};

  /**
   * @brief Construct a Vec from a pair of iterators.
   * @param first iterator pointing to the first element
   * @param last iterator pointing to the one past last element
   * @tparam It iterator type
   */
  template <typename It>
  explicit Vec(It first, It last) : m_values(first, last) {}

  /**
   * @brief The size of the Vec.
   */
  std::size_t Size() const {
    return m_values.size();
  };

  /**
   * @brief Check if this Vec is empty.
   */
  bool Empty() const {
    return Size() == 0;
  };

  /**
   * @brief Mutable access to vector elements.
   */
  Elem& operator[](std::size_t idx) {
    return m_values[idx];
  };

  /**
   * @brief Read only access to vector elements.
   */
  Elem operator[](std::size_t idx) const {
    return m_values[idx];
  };

  /**
   * @brief Add two Vec objects entry-wise.
   * @param other the other vector
   * @return the sum of this and \p other.
   */
  Vec Add(const Vec& other) const;

  /**
   * @brief Add two Vec objects entry-wise in-place.
   * @param other the other vector
   * @return the sum of this and \p other, assigned to this.
   */
  Vec& AddInPlace(const Vec& other) {
    EnsureCompatible(other);
    for (std::size_t i = 0; i < Size(); i++) {
      m_values[i] += other.m_values[i];
    }
    return *this;
  };

  /**
   * @brief Subtract two Vec objects entry-wise.
   * @param other the other vector
   * @return the difference of this and \p other.
   */
  Vec Subtract(const Vec& other) const;

  /**
   * @brief Subtract two Vec objects entry-wise in-place.
   * @param other the other vector
   * @return the difference of this and \p other, assigned to this.
   */
  Vec& SubtractInPlace(const Vec& other) {
    EnsureCompatible(other);
    for (std::size_t i = 0; i < Size(); i++) {
      m_values[i] -= other.m_values[i];
    }
    return *this;
  };

  /**
   * @brief Multiply two Vec objects entry-wise.
   * @param other the other vector
   * @return the product of this and \p other.
   */
  Vec MultiplyEntryWise(const Vec& other) const;

  /**
   * @brief Multiply two Vec objects entry-wise in-place.
   * @param other the other vector
   * @return the product of this and \p other, assigned to this.
   */
  Vec& MultiplyEntryWiseInPlace(const Vec& other) {
    EnsureCompatible(other);
    for (std::size_t i = 0; i < Size(); i++) {
      m_values[i] *= other.m_values[i];
    }
    return *this;
  };

  /**
   * @brief Compute a dot product between this and another vector.
   * @param other the other vector
   * @return the dot (or inner) product of this and \p other.
   */
  Elem Dot(const Vec& other) const {
    EnsureCompatible(other);
    return UncheckedInnerProd<Elem>(begin(), end(), other.begin());
  };

  /**
   * @brief Compute the sum over entries of this vector.
   * @return the sum of the entries of this vector.
   */
  Elem Sum() const {
    Elem sum;
    for (const auto& v : m_values) {
      sum += v;
    }
    return sum;
  };

  /**
   * @brief Scale this vector by a constant.
   * @param scalar the scalar
   * @return a scaled version of this vector.
   */
  Vec ScalarMultiply(const Elem& scalar) const {
    std::vector<Elem> r;
    r.reserve(Size());
    for (const auto& v : m_values) {
      r.emplace_back(scalar * v);
    }
    return Vec(r);
  };

  /**
   * @brief Scale this vector in-place by a constant.
   * @param scalar the scalar
   * @return a scaled version of this vector.
   */
  Vec& ScalarMultiplyInPlace(const Elem& scalar) {
    for (auto& v : m_values) {
      v *= scalar;
    }
    return *this;
  };

  /**
   * @brief Test if this vector is equal to another vector in constant time.
   * @param other the other vector
   * @return true if this vector is equal to \p other and false otherwise.
   */
  bool Equals(const Vec& other) const;

  /**
   * @brief Operator == overload for Vec.
   */
  friend bool operator==(const Vec& left, const Vec& right) {
    return left.Equals(right);
  };

  /**
   * @brief Operator != overload for Vec.
   */
  friend bool operator!=(const Vec& left, const Vec& right) {
    return !(left == right);
  }

  /**
   * @brief Convert this vector into a 1-by-N row matrix.
   */
  Mat<Elem> ToRowMatrix() const {
    return Mat<Elem>{1, Size(), m_values};
  }

  /**
   * @brief Convert this vector into a N-by-1 column matrix.
   */
  Mat<Elem> ToColumnMatrix() const {
    return Mat<Elem>{Size(), 1, m_values};
  }

  /**
   * @brief Convert this Vec object to an std::vector.
   */
  std::vector<Elem>& ToStlVector() {
    return m_values;
  }

  /**
   * @brief Convert this Vec object to a const std::vector.
   */
  const std::vector<Elem>& ToStlVector() const {
    return m_values;
  }

  /**
   * @brief Extract a sub-vector
   * @param start the start index, inclusive
   * @param end the end index, exclusive
   * @return a sub-vector.
   */
  Vec<Elem> SubVector(std::size_t start, std::size_t end) const {
    if (start > end) {
      throw std::logic_error("invalid range");
    }
    return Vec<Elem>(begin() + start, begin() + end);
  }

  /**
   * @brief Extract a sub-vector.
   *
   * This method is equivalent to <code>Vec#SubVector(0, end)</code>.
   *
   * @param end the end index, exclusive
   * @return a sub-vector.
   */
  Vec<Elem> SubVector(std::size_t end) const {
    return SubVector(0, end);
  }

  /**
   * @brief Return a string representation of this vector.
   */
  std::string ToString() const;

  /**
   * @brief Write a string representation of this vector to a stream.
   */
  friend std::ostream& operator<<(std::ostream& os, const Vec<Elem>& v) {
    return os << v.ToString();
  }

  /**
   * @brief Write this Vec to a buffer.
   * @param dest the buffer to write this Vec to
   */
  void Write(unsigned char* dest) const;

  /**
   * @brief Returns the number of bytes that Write writes.
   */
  std::size_t ByteSize() const {
    return Size() * Elem::ByteSize();
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
  void EnsureCompatible(const Vec& other) const {
    if (Size() != other.Size()) {
      throw std::invalid_argument("Vec sizes mismatch");
    }
  }

  std::vector<Elem> m_values;
};

template <typename Elem>
Vec<Elem> Vec<Elem>::Read(std::size_t n, const unsigned char* src) {
  std::vector<Elem> r;
  r.reserve(n);
  std::size_t offset = 0;
  while (n-- > 0) {
    r.emplace_back(Elem::Read(src + offset));
    offset += Elem::ByteSize();
  }
  return Vec<Elem>(r);
}

template <typename Elem>
Vec<Elem> Vec<Elem>::Range(std::size_t start, std::size_t end) {
  if (start > end) {
    throw std::invalid_argument("invalid range");
  }
  if (start == end) {
    return Vec<Elem>{};
  }

  std::vector<Elem> v;
  v.reserve(end - start);
  for (std::size_t i = start; i < end; ++i) {
    v.emplace_back(Elem{(int)i});
  }
  return Vec<Elem>(v);
}

template <typename Elem>
Vec<Elem> Vec<Elem>::Random(std::size_t n, util::PRG& prg) {
  auto buf = std::make_unique<unsigned char[]>(n * Elem::ByteSize());
  prg.Next(buf.get(), n * Elem::ByteSize());

  std::vector<Elem> elements;
  elements.reserve(n);
  for (std::size_t i = 0; i < n; ++i) {
    elements.emplace_back(Elem::Read(buf.get() + i * Elem::ByteSize()));
  }

  return Vec<Elem>(elements);
}

template <typename Elem>
Vec<Elem> Vec<Elem>::Add(const Vec<Elem>& other) const {
  EnsureCompatible(other);
  std::vector<Elem> r;
  auto n = Size();
  r.reserve(n);
  for (std::size_t i = 0; i < n; i++) {
    r.emplace_back(m_values[i] + other.m_values[i]);
  }
  return Vec(r);
}

template <typename Elem>
Vec<Elem> Vec<Elem>::Subtract(const Vec<Elem>& other) const {
  EnsureCompatible(other);
  std::vector<Elem> r;
  auto n = Size();
  r.reserve(n);
  for (std::size_t i = 0; i < n; i++) {
    r.emplace_back(m_values[i] - other.m_values[i]);
  }
  return Vec(r);
}

template <typename Elem>
Vec<Elem> Vec<Elem>::MultiplyEntryWise(const Vec<Elem>& other) const {
  EnsureCompatible(other);
  std::vector<Elem> r;
  auto n = Size();
  r.reserve(n);
  for (std::size_t i = 0; i < n; i++) {
    r.emplace_back(m_values[i] * other.m_values[i]);
  }
  return Vec(r);
}

template <typename Elem>
bool Vec<Elem>::Equals(const Vec<Elem>& other) const {
  if (Size() != other.Size()) {
    return false;
  }

  bool equal = true;
  for (std::size_t i = 0; i < Size(); i++) {
    equal &= m_values[i] == other.m_values[i];
  }

  return equal;
}

template <typename Elem>
std::string Vec<Elem>::ToString() const {
  if (Empty()) {
    return "[ EMPTY VECTOR ]";
  }

  std::stringstream ss;
  ss << "[";
  std::size_t i = 0;
  for (; i < Size() - 1; i++) {
    ss << m_values[i] << ", ";
  }
  ss << m_values[i] << "]";
  return ss.str();
}

template <typename Elem>
void Vec<Elem>::Write(unsigned char* dest) const {
  for (const auto& v : m_values) {
    v.Write(dest);
    dest += Elem::ByteSize();
  }
}

}  // namespace scl::math

#endif  // SCL_MATH_VEC_H
