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

#ifndef SCL_MATH_POLY_H
#define SCL_MATH_POLY_H

#include <array>

#include "scl/math/vector.h"

namespace scl::math {

/**
 * @brief Polynomials over rings.
 */
template <typename RING>
class Polynomial {
 public:
  /**
   * @brief Construct a polynomial with some supplied coefficients.
   * @param coefficients the coefficients
   *
   */
  static Polynomial<RING> create(const Vector<RING>& coefficients);

  /**
   * @brief Construct a constant polynomial with constant term 0.
   */
  Polynomial() : m_coefficients(1) {}

  /**
   * @brief Construct a constant polynomial.
   * @param constant the constant term of the polynomial
   */
  Polynomial(const RING& constant) : m_coefficients({constant}) {}

  /**
   * @brief Evaluate this polynomial on a supplied point.
   * @param x the point to evaluate this polynomial on
   * @return f(x) where \p x is the supplied point and f this polynomial.
   */
  RING evaluate(const RING& x) const {
    auto it = m_coefficients.rbegin();
    auto end = m_coefficients.rend();
    auto y = *it++;
    while (it != end) {
      y = *it++ + y * x;
    }
    return y;
  }  // LCOV_EXCL_LINE

  /**
   * @brief Access coefficients, with the constant term at position 0.
   */
  RING& operator[](std::size_t idx) {
    return m_coefficients[idx];
  }

  /**
   * @brief Access coefficients, with the constant term at position 0.
   */
  RING operator[](std::size_t idx) const {
    return m_coefficients[idx];
  }

  /**
   * @brief Get the coefficients of this polynomial.
   * @return the coefficients.
   */
  Vector<RING> coefficients() const {
    return m_coefficients;
  }

  /**
   * @brief Add two polynomials.
   */
  Polynomial add(const Polynomial& q) const;

  /**
   * @brief Subtraction two polynomials.
   */
  Polynomial subtract(const Polynomial& q) const;

  /**
   * @brief Multiply two polynomials.
   */
  Polynomial multiply(const Polynomial& q) const;

  /**
   * @brief Divide two polynomials.
   * @return A pair \f$(q, r)\f$ such that \f$\mathtt{this} = p * q + r\f$.
   */
  std::array<Polynomial, 2> divide(const Polynomial& q) const;

  /**
   * @brief Returns true if this is the 0 polynomial.
   */
  bool isZero() const {
    return degree() == 0 && constantTerm() == RING(0);
  }

  /**
   * @brief Get the constant term of this polynomial.
   */
  RING constantTerm() const {
    return operator[](0);
  }

  /**
   * @brief Get the leading term of this polynomial.
   */
  RING leadingTerm() const {
    return operator[](degree());
  }

  /**
   * @brief Degree of this polynomial.
   */
  std::size_t degree() const {
    return m_coefficients.size() - 1;
  }

  /**
   * @brief Check if two polynomials are equal.
   */
  friend bool operator==(const Polynomial& p, const Polynomial& q) {
    return p.m_coefficients == q.m_coefficients;
  }

  /**
   * @brief Get a string representation of this polynomial.
   *
   * If the coefficients of \p this is <code>[1, 2, 3]</code> then
   * <code>this.ToString("f", "x")</code> will return the string "f(x) = 1 + 2x
   * + 3x^2".
   *
   * @param polynomial_name the string to use for the name of the polynomial
   * @param variable_name the string to use for the variable name
   */
  std::string toString(const char* polynomial_name,
                       const char* variable_name) const;

  /**
   * @brief Get a string representation of this polynomial.
   * @note Equivalent to ToString("f", "x").
   */
  std::string toString() const {
    return toString("f", "x");
  }

  /**
   * @brief Write a string representation of this polynomial to a stream.
   */
  friend std::ostream& operator<<(std::ostream& os, const Polynomial& p) {
    return os << p.toString();
  }

 private:
  Polynomial(const Vector<RING>& coefficients) : m_coefficients(coefficients){};

  Vector<RING> m_coefficients;
};

template <typename RING>
Polynomial<RING> Polynomial<RING>::create(const Vector<RING>& coefficients) {
  auto it = coefficients.rbegin();
  auto end = coefficients.rend();
  auto cutoff = coefficients.size();
  RING zero;
  for (; it != end; ++it) {
    if (*it != zero) {
      break;
    }
    --cutoff;
  }
  const auto c =
      Vector<RING>(coefficients.begin(), coefficients.begin() + cutoff);

  if (c.empty()) {
    return Polynomial<RING>{};
  }

  return Polynomial<RING>{c};
}

/**
 * @brief Pads the coefficients of a polynomial with zeros.
 * @param p the polynomial
 * @param n the size of the final Vec
 * @return A scl::Vec of length \p n with coefficients of p and zeros.
 */
template <typename RING>
Vector<RING> padCoefficients(const Polynomial<RING>& p, std::size_t n) {
  Vector<RING> c(n);
  for (std::size_t i = 0; i < n; ++i) {
    if (i <= p.degree()) {
      c[i] = p[i];
    }
  }
  return c;
}  // LCOV_EXCL_LINE

template <typename RING>
Polynomial<RING> Polynomial<RING>::add(const Polynomial<RING>& q) const {
  const auto this_larger = degree() > q.degree();
  const auto n = (this_larger ? degree() : q.degree()) + 1;
  const auto pp = padCoefficients(*this, n);
  const auto qp = padCoefficients(q, n);
  const auto c = pp.add(qp);
  return Polynomial<RING>::create(c);
}

template <typename RING>
Polynomial<RING> Polynomial<RING>::subtract(const Polynomial<RING>& q) const {
  const auto this_larger = degree() > q.degree();
  const auto n = (this_larger ? degree() : q.degree()) + 1;
  const auto pp = padCoefficients(*this, n);
  const auto qp = padCoefficients(q, n);
  const auto c = pp.subtract(qp);
  return Polynomial<RING>::create(c);
}

template <typename RING>
Polynomial<RING> Polynomial<RING>::multiply(const Polynomial<RING>& q) const {
  Vector<RING> c(degree() + q.degree() + 1);
  for (std::size_t i = 0; i <= degree(); ++i) {
    for (std::size_t j = 0; j <= q.degree(); ++j) {
      c[i + j] += operator[](i) * q[j];
    }
  }
  return Polynomial<RING>::create(c);
}

/**
 * @brief Divide the leading terms of two polynomials.
 * @note assumes that <code>deg(p) >= deg(q)</code>.
 */
template <typename RING>
Polynomial<RING> divideLeadingTerms(const Polynomial<RING>& p,
                                    const Polynomial<RING>& q) {
  const auto deg_out = p.degree() - q.degree();
  Vector<RING> c(deg_out + 1);
  c[deg_out] = p.leadingTerm() / q.leadingTerm();
  return Polynomial<RING>::create(c);
}

template <typename RING>
std::array<Polynomial<RING>, 2> Polynomial<RING>::divide(
    const Polynomial<RING>& q) const {
  if (q.isZero()) {
    throw std::invalid_argument("division by 0");
  }

  // https://en.wikipedia.org/wiki/Polynomial_long_division#Pseudocode

  Polynomial p;
  Polynomial r = *this;
  while (!r.isZero() && r.degree() >= q.degree()) {
    const auto t = divideLeadingTerms(r, q);
    p = p.add(t);
    r = r.subtract(t.multiply(q));
  }
  return {p, r};
}

template <typename RING>
std::string Polynomial<RING>::toString(const char* polynomial_name,
                                       const char* variable_name) const {
  std::stringstream ss;
  ss << polynomial_name << "(" << variable_name << ") = " << m_coefficients[0];
  for (std::size_t i = 1; i < m_coefficients.size(); i++) {
    ss << " + " << m_coefficients[i] << variable_name;
    if (i > 1) {
      ss << "^" << i;
    }
  }
  return ss.str();
}

}  // namespace scl::math

#endif  // SCL_MATH_POLY_H
