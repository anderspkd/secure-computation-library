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

#ifndef SCL_NET_CHANNEL_H
#define SCL_NET_CHANNEL_H

#include <cstring>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include "scl/math/mat.h"
#include "scl/math/vec.h"
#include "scl/net/config.h"
#include "scl/util/traits.h"

namespace scl::net {

/**
 * @brief The maximum size of a Vec that a Channel will receive.
 *
 * This is mostly meant as a guard against programming mistakes. The default
 * value should allow receiving up to around 500mb assuming element sizes of 16
 * bytes.
 */
#ifndef MAX_VEC_READ_SIZE
#define MAX_VEC_READ_SIZE 1 << 25
#endif

/**
 * @brief The maxium size of a Mat that a channel will receive.
 *
 * This is mostly meant as a guard against programming mistakes. The default
 * value should allow receiving up to around 500mb assuming element sizes of 16
 * bytes.
 */
#ifndef MAX_MAT_READ_SIZE
#define MAX_MAT_READ_SIZE 1 << 25
#endif

#define SCL_CC(x) reinterpret_cast<const unsigned char(*)>((x))
#define SCL_C(x) reinterpret_cast<unsigned char(*)>((x))

/**
 * @brief Channel interface.
 *
 * Channel defines the interface for a channel between two peers, as well
 * as a number of convenience methods for sending and receiving different kinds
 * of data. To implement an actual channel, subclass Channel and implement
 * the four virtual methods.
 *
 * @see InMemoryChannel
 * @see TcpChannel
 */
class Channel {
 public:
  virtual ~Channel(){};

  /**
   * @brief Close connection to remote.
   */
  virtual void Close() = 0;

  /**
   * @brief Send data to the remote party.
   * @param src the data to send
   * @param n the number of bytes to send
   */
  virtual void Send(const unsigned char* src, std::size_t n) = 0;

  /**
   * @brief Receive data from the remote party.
   * @param dst where to store the received data
   * @param n how much data to receive
   * @return how many bytes were received.
   */
  virtual std::size_t Recv(unsigned char* dst, std::size_t n) = 0;

  /**
   * @brief Check if there is something to receive on this channel.
   * @return true if this channel has data and false otherwise.
   */
  virtual bool HasData() = 0;

  /**
   * @brief Send a trivially copyable item.
   * @param src the thing to send
   */
  template <
      typename T,
      typename std::enable_if_t<std::is_trivially_copyable_v<T>, bool> = true>
  void Send(const T& src) {
    Send(SCL_CC(&src), sizeof(T));
  }

  /**
   * @brief Send a vector of trivially copyable things.
   * @param src an STL vector of things to send
   */
  template <
      typename T,
      typename std::enable_if_t<std::is_trivially_copyable_v<T>, bool> = true>
  void Send(const std::vector<T>& src) {
    Send(src.size());
    Send(SCL_CC(src.data()), sizeof(T) * src.size());
  }

  /**
   * @brief Send a Vec object.
   * @param vec the Vec
   */
  template <typename T>
  void Send(const math::Vec<T>& vec) {
    const std::uint32_t vec_size = vec.Size();
    Send(vec_size);
    // have to make a copy here since it's not guaranteed that we can directly
    // write T to the channel.
    auto buf = std::make_unique<unsigned char[]>(vec.ByteSize());
    vec.Write(buf.get());
    Send(SCL_CC(buf.get()), vec.ByteSize());
  }

  /**
   * @brief Send a Mat object.
   * @param mat the Mat
   */
  template <typename T>
  void Send(const math::Mat<T>& mat) {
    const std::uint32_t rows = mat.Rows();
    const std::uint32_t cols = mat.Cols();
    Send(rows);
    Send(cols);
    auto buf = std::make_unique<unsigned char[]>(mat.ByteSize());
    mat.Write(buf.get());
    Send(SCL_CC(buf.get()), mat.ByteSize());
  }

  /**
   * @brief Receive a trivially copyable item.
   * @param dst where to store the received item
   */
  template <
      typename T,
      typename std::enable_if_t<std::is_trivially_copyable_v<T>, bool> = true>
  void Recv(T& dst) {
    Recv(SCL_C(&dst), sizeof(T));
  }

  /**
   * @brief Receive a vector of trivially copyable items.
   * @param dst where to store the received items
   * @note any existing content in \p dst is overwritten.
   */
  template <
      typename T,
      typename std::enable_if_t<std::is_trivially_copyable_v<T>, bool> = true>
  void Recv(std::vector<T>& dst) {
    std::size_t size;
    Recv(size);
    dst.resize(size);
    Recv(SCL_C(dst.data()), sizeof(T) * size);
  }

  /**
   * @brief Receive a vector.
   * @param vec where to store the received vector
   * @throws std::logic_error in case the received vector size exceeds
   * MAX_VEC_READ_SIZE
   */
  template <typename T>
  void Recv(math::Vec<T>& vec) {
    auto vec_size = RecvSize();
    if (vec_size > MAX_VEC_READ_SIZE) {
      throw std::logic_error("received vector exceeds size limit");
    }
    auto n = vec_size * T::ByteSize();
    auto buf = std::make_unique<unsigned char[]>(n);
    Recv(SCL_C(buf.get()), n);
    vec = math::Vec<T>::Read(vec_size, SCL_CC(buf.get()));
  }

  /**
   * @brief Receive a Matrix.
   * @param mat where to store the received matrix
   * @throws std::logic_error in case the row count times column count exceeds
   * MAX_MAT_READ_SIZE
   */
  template <typename T>
  void Recv(math::Mat<T>& mat) {
    auto rows = RecvSize();
    auto cols = RecvSize();
    if (rows * cols > MAX_MAT_READ_SIZE) {
      throw std::logic_error("received matrix exceeds size limit");
    }
    auto n = rows * cols * T::ByteSize();
    auto buf = std::make_unique<unsigned char[]>(n);
    Recv(SCL_C(buf.get()), n);
    mat = math::Mat<T>::Read(rows, cols, SCL_CC(buf.get()));
  }

 private:
  std::uint32_t RecvSize() {
    std::uint32_t size;
    Recv(size);
    return size;
  }
};

#undef SCL_C
#undef SCL_CC

}  // namespace scl::net

#endif  // SCL_NET_CHANNEL_H
