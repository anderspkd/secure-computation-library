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

#ifndef SCL_SIMULATION_BUFFER_H
#define SCL_SIMULATION_BUFFER_H

#include <vector>

namespace scl::sim {

/**
 * @brief Peer-to-peer channel interface for passing data in simulations.
 *
 * ChannelBuffer is interface of any communication channel between two peers in
 * a simulation. Besides allowing reads and writes, a ChannelBuffer must also
 * support Prepare-Commit-Rollback logic. This is needed because in case of
 * simulation failures, it is necessary to undo any reads and writes performed
 * on a channel.
 */
struct ChannelBuffer {
  virtual ~ChannelBuffer(){};

  /**
   * @brief Read data from the channel.
   * @param data the data to write.
   * @param n the number of bytes to read.
   */
  virtual void Read(unsigned char* data, std::size_t n) = 0;

  /**
   * @brief Write data to the channel.
   * @param data the data to write.
   * @param n the number of bytes to write.
   */
  virtual void Write(const unsigned char* data, std::size_t n) = 0;

  /**
   * @brief Get the amount of bytes that can be read from this channel.
   */
  virtual std::size_t Size() = 0;

  /**
   * @brief Prepare reads and writes.
   */
  virtual void Prepare() = 0;

  /**
   * @brief Commit reads and writes.
   */
  virtual void Commit() = 0;

  /**
   * @brief Rollback reads and writes.
   */
  virtual void Rollback() = 0;
};

}  // namespace scl::sim

#endif  // SCL_SIMULATION_BUFFER_H
