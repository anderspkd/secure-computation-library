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

#ifndef SCL_MATH_POLY_H
#define SCL_MATH_POLY_H

#include <array>

#include "scl/math/vec.h"

namespace scl::math {

/**
 * @brief Polynomials over finite fields.
 */
template <typename T>
class Polynomial {
 public:
  /**
   * @brief Construct a polynomial with some supplied coefficients.
   * @param coefficients the coefficients
   *
   */
  static Polynomial<T> Create(const Vec<T>& coefficients);

  /**
   * @brief Construct a constant polynomial with constant term 0.
   */
  Polynomial() : m_coefficients(1) {}

  /**
   * @brief Construct a constant polynomial.
   * @param constant the constant term of the polynomial
   */
  Polynomial(const T& constant) : m_coefficients({constant}) {}

  /**
   * @brief Evaluate this polynomial on a supplied point.
   * @param x the point to evaluate this polynomial on
   * @return f(x) where \p x is the supplied point and f this polynomial.
   */
  T Evaluate(const T& x) const {
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
  T& operator[](std::size_t idx) {
    return m_coefficients[idx];
  }

  /**
   * @brief Access coefficients, with the constant term at position 0.
   */
  T operator[](std::size_t idx) const {
    return m_coefficients[idx];
  }

  /**
   * @brief Get the coefficients of this polynomial.
   * @return the coefficients.
   */
  Vec<T> Coefficients() const {
    return m_coefficients;
  }

  /**
   * @brief Add two polynomials.
   */
  Polynomial Add(const Polynomial& q) const;

  /**
   * @brief Subtraction two polynomials.
   */
  Polynomial Subtract(const Polynomial& q) const;

  /**
   * @brief Multiply two polynomials.
   */
  Polynomial Multiply(const Polynomial& q) const;

  /**
   * @brief Divide two polynomials.
   * @return A pair \f$(q, r)\f$ such that \f$\mathtt{this} = p * q + r\f$.
   */
  std::array<Polynomial, 2> Divide(const Polynomial& q) const;

  /**
   * @brief Returns true if this is the 0 polynomial.
   */
  bool IsZero() const {
    return Degree() == 0 && ConstantTerm() == T(0);
  }

  /**
   * @brief Get the constant term of this polynomial.
   */
  T ConstantTerm() const {
    return operator[](0);
  }

  /**
   * @brief Get the leading term of this polynomial.
   */
  T LeadingTerm() const {
    return operator[](Degree());
  }

  /**
   * @brief Degree of this polynomial.
   */
  std::size_t Degree() const {
    return m_coefficients.Size() - 1;
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
  std::string ToString(const char* polynomial_name,
                       const char* variable_name) const;

  /**
   * @brief Get a string representation of this polynomial.
   * @note Equivalent to ToString("f", "x").
   */
  std::string ToString() const {
    return ToString("f", "x");
  }

  /**
   * @brief Write a string representation of this polynomial to a stream.
   */
  friend std::ostream& operator<<(std::ostream& os, const Polynomial& p) {
    return os << p.ToString();
  }

 private:
  Polynomial(const Vec<T>& coefficients) : m_coefficients(coefficients){};

  Vec<T> m_coefficients;
};

template <typename T>
Polynomial<T> Polynomial<T>::Create(const Vec<T>& coefficients) {
  auto it = coefficients.rbegin();
  auto end = coefficients.rend();
  auto cutoff = coefficients.Size();
  T zero;
  for (; it != end; ++it) {
    if (*it != zero) {
      break;
    }
    --cutoff;
  }
  const auto c = Vec<T>(coefficients.begin(), coefficients.begin() + cutoff);

  if (c.Empty()) {
    return Polynomial<T>{};
  }

  return Polynomial<T>{c};
}

/**
 * @brief Pads the coefficients of a polynomial with zeros.
 * @param p the polynomial
 * @param n the size of the final Vec
 * @return A scl::Vec of length \p n with coefficients of p and zeros.
 */
template <typename T>
Vec<T> PadCoefficients(const Polynomial<T>& p, std::size_t n) {
  Vec<T> c(n);
  for (std::size_t i = 0; i < n; ++i) {
    if (i <= p.Degree()) {
      c[i] = p[i];
    }
  }
  return c;
}  // LCOV_EXCL_LINE

template <typename T>
Polynomial<T> Polynomial<T>::Add(const Polynomial<T>& q) const {
  const auto this_larger = Degree() > q.Degree();
  const auto n = (this_larger ? Degree() : q.Degree()) + 1;
  const auto pp = PadCoefficients(*this, n);
  const auto qp = PadCoefficients(q, n);
  const auto c = pp.Add(qp);
  return Polynomial<T>::Create(c);
}

template <typename T>
Polynomial<T> Polynomial<T>::Subtract(const Polynomial<T>& q) const {
  const auto this_larger = Degree() > q.Degree();
  const auto n = (this_larger ? Degree() : q.Degree()) + 1;
  const auto pp = PadCoefficients(*this, n);
  const auto qp = PadCoefficients(q, n);
  const auto c = pp.Subtract(qp);
  return Polynomial<T>::Create(c);
}

template <typename T>
Polynomial<T> Polynomial<T>::Multiply(const Polynomial<T>& q) const {
  Vec<T> c(Degree() + q.Degree() + 1);
  for (std::size_t i = 0; i <= Degree(); ++i) {
    for (std::size_t j = 0; j <= q.Degree(); ++j) {
      c[i + j] += operator[](i) * q[j];
    }
  }
  return Polynomial<T>::Create(c);
}

/**
 * @brief Divide the leading terms of two polynomials.
 * @note assumes that <code>deg(p) >= deg(q)</code>.
 */
template <typename T>
Polynomial<T> DivideLeadingTerms(const Polynomial<T>& p,
                                 const Polynomial<T>& q) {
  const auto deg_out = p.Degree() - q.Degree();
  Vec<T> c(deg_out + 1);
  c[deg_out] = p.LeadingTerm() / q.LeadingTerm();
  return Polynomial<T>::Create(c);
}

template <typename T>
std::array<Polynomial<T>, 2> Polynomial<T>::Divide(
    const Polynomial<T>& q) const {
  if (q.IsZero()) {
    throw std::invalid_argument("division by 0");
  }

  // https://en.wikipedia.org/wiki/Polynomial_long_division#Pseudocode

  Polynomial p;
  Polynomial r = *this;
  while (!r.IsZero() && r.Degree() >= q.Degree()) {
    const auto t = DivideLeadingTerms(r, q);
    p = p.Add(t);
    r = r.Subtract(t.Multiply(q));
  }
  return {p, r};
}

template <typename T>
std::string Polynomial<T>::ToString(const char* polynomial_name,
                                    const char* variable_name) const {
  std::stringstream ss;
  ss << polynomial_name << "(" << variable_name << ") = " << m_coefficients[0];
  for (std::size_t i = 1; i < m_coefficients.Size(); i++) {
    ss << " + " << m_coefficients[i] << variable_name;
    if (i > 1) {
      ss << "^" << i;
    }
  }
  return ss.str();
}

}  // namespace scl::math

#endif  // SCL_MATH_POLY_H
