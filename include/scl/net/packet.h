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

#ifndef SCL_NET_PACKET_H
#define SCL_NET_PACKET_H

#include <cstddef>
#include <cstdlib>
#include <memory>

#include "scl/serialization/serializer.h"
#include "scl/serialization/serializers.h"

namespace scl::net {

/**
 * @brief A container for data to be sent on a Channel.
 *
 * <p>A Packet is container that serializes all writes and deserializes all
 * reads. Packet can therefore be used to construct content that is meant to be
 * sent over a Channel. Example usage:
 *
 * @code
 * net::Packet p;
 * p << (int)1234;                      // write an int to the packet.
 * // p.Write((int)1234);               // same as above.
 * p << math::FF<T>{42};                // write a FF element.
 *
 * std::cout << p.Size() << std::endl;  // size of a packet. This will be the
 *                                      // size in bytes of the content written
 *                                      // to it, so with the values written
 *                                      // above, it will be something like
 *                                      // sizeof(int) + math::FF<T>::ByteSize()
 *
 * auto v = p.Read<int>();              // read the integer written above
 * auto f = p.Read<math::FF<T>>();      // read the FF element.
 * @endcode
 *
 * <p>Packet is essentially a struct with the following format
 *
 * @code
 * struct Packet {
 *   unsigned char* buffer;    // data
 *   std::size_t cap;          // number of bytes allocated
 *   std::ptrdiff_t read_ptr;  // pointer into buffer
 *   std::ptrdiff_t write_ptr; // pointer into buffer
 * };
 * @endcode
 */
class Packet {
 private:
  // the internal buffer is resized using realloc(3) so it has to be allocated
  // using malloc(3) and deallocated using free(3), as opposed to new/delete.
  struct FreeDeleter {
    void operator()(void* p) const {
      std::free(p);
    }
  };

  // Internal buffer type.
  using Buffer = std::unique_ptr<unsigned char[], FreeDeleter>;

 public:
  /**
   * @brief Type used to denote the size of a packet.
   *
   * A type with a guaranteed size is used here to avoid issues where e.g.,
   * <code>std::size_t</code> has a different size on the sender vs. the
   * receiver.
   */
  using SizeType = std::uint32_t;

  /**
   * @brief Construct a new packet.
   * @param initial_size the initial amount of bytes to allocate.
   */
  Packet(std::size_t initial_size = 2048)
      : m_buffer(static_cast<unsigned char*>(std::malloc(initial_size))),
        m_cap(initial_size),
        m_read_ptr(0),
        m_write_ptr(0) {}

  /**
   * @brief Read an object from the packet.
   * @tparam T the type of the object to read.
   *
   * This function reads the next object from the packet using an appropriate
   * util::Serializer. A specialization of util::Serialization for \p T must
   * therefore exist.
   */
  template <typename T>
  T Read() {
    T v;
    const auto sz = seri::Serializer<T>::Read(v, Get() + m_read_ptr);
    m_read_ptr += sz;
    return v;
  }  // LCOV_EXCL_LINE

  /**
   * @brief Write an object to this packet.
   * @param obj the object to read.
   *
   * This function writes \p obj using an util::Serializer. Calling this
   * function may also result in the internal buffer being resized.
   */
  template <typename T>
  void Write(const T& obj) {
    const auto sz = seri::Serializer<T>::SizeOf(obj);
    ReserveSpace(sz);
    seri::Serializer<T>::Write(obj, Get() + m_write_ptr);
    m_write_ptr += sz;
  }

  /**
   * @brief Operator << support for writing to the packet.
   */
  template <typename T>
  friend Packet& operator<<(Packet& packet, const T& thing) {
    packet.Write(thing);
    return packet;
  }

  /**
   * @brief The size of a packet.
   */
  SizeType Size() const {
    return m_write_ptr;
  }

  /**
   * @brief Get a raw const pointer to the content of this packet.
   */
  const unsigned char* Get() const {
    return m_buffer.get();
  }

  /**
   * @brief Get a raw pointer to the conte of this packet.
   */
  unsigned char* Get() {
    return m_buffer.get();
  }

  /**
   * @brief Set the internal write pointer.
   *
   * This function effectively resizes the packet to a smaller size, and can
   * therefore be used to overwrite existing content. Calling this function with
   * <code>new_write_ptr > Size()</code> may result in reading outside the
   * internal buffer, so don't do this.
   */
  void SetWritePtr(std::ptrdiff_t new_write_ptr) {
    m_write_ptr = new_write_ptr;
    m_read_ptr = std::min(m_write_ptr, m_read_ptr);
  }

  /**
   * @brief Resets the internal read pointer.
   *
   * Since calls to Read are stateful, this function can be used to re-read the
   * content of a packet.
   */
  void ResetWritePtr() {
    SetWritePtr(0);
  }

  /**
   * @brief Set the internal read pointer.
   *
   * This function allows moving the read pointer around, and can therefore be
   * used skipping objects, or re-reading objects. Only valid for
   * <code>new_read_ptr < Size()</code>.
   */
  void SetReadPtr(std::ptrdiff_t new_read_ptr) {
    m_read_ptr = new_read_ptr;
  }

  /**
   * @brief Resets the internal write pointer.
   *
   * Like reads, writes are also stateful. This function allows reusing an
   * existing packet.
   */
  void ResetReadPtr() {
    SetReadPtr(0);
  }

 private:
  void ResizeBuffer(std::size_t new_size) {
    auto* buf = m_buffer.release();
    auto* buf_new = std::realloc(buf, new_size);
    if (buf_new == nullptr) {
      // Not testing this at the moment. It would be better if allocation of the
      // internal buffer has handled by an allocator type of some sort. Possibly
      // one from the standard library.
      throw std::bad_alloc();  // LCOV_EXCL_LINE
    }
    m_buffer.reset(static_cast<unsigned char*>(buf_new));
    m_cap = new_size;
  }

  void ReserveSpace(std::size_t obj_size) {
    const auto min_size = obj_size + m_write_ptr;
    if (min_size > m_cap) {
      const auto new_size = std::max(min_size, 2 * m_cap);
      ResizeBuffer(new_size);
    }
  }

  Buffer m_buffer;
  std::size_t m_cap;
  std::ptrdiff_t m_read_ptr;
  std::ptrdiff_t m_write_ptr;
};

}  // namespace scl::net

#endif  // SCL_NET_PACKET_H
