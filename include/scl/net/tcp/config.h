/* SCL --- Secure Computation Library
 * Copyright (C) 2025 Anders Dalskov
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

#ifndef SCL_NET_CONFIG_H
#define SCL_NET_CONFIG_H

#include <cstddef>
#include <string>
#include <vector>

/**
 * @brief Default port offset used when all parties are running locally.
 * @ingroup net-tcp
 */
#ifndef DEFAULT_LOCALHOST_PORT_OFFSET
#define DEFAULT_LOCALHOST_PORT_OFFSET 9900
#endif

namespace scl {

/**
 * @brief Connection information for a party.
 * @ingroup net-tcp
 */
struct ConnectionInfo {
  /**
   * @brief The id of this party.
   */
  std::size_t id;

  /**
   * @brief The hostname.
   */
  std::string hostname;

  /**
   * @brief The port.
   */
  std::size_t port;
};

/**
 * @brief Network configuration.
 * @ingroup net-tcp
 *
 * A NetworkConfig is needed whenever by network objects in order to establish
 * connections to other nodes.
 *
 * @see NetworkConfig::load
 * @see NetworkConfig::localhost
 */
class NetworkConfig {
 public:
  /**
   * @brief Load a network config from a file.
   *
   * This method creates a NetworkConfig from a file containing connection
   * information of the different parties. The file should be a CSV filed with
   * three columns. Each row in the file contains a party's IPv4 address and the
   * port to use, in that order. An example is given below for three parties
   *
   * \code{.unparsed}
   * $ cat 3_parties.txt
   * 192.0.2.1, 8000
   * 192.0.2.2, 5000
   * 192.0.2.3, 3000
   * $
   * \endcode
   *
   * The order in which parties information occurs will correspond to their
   * identifier in the NetworkConfig that will be created.
   *
   * @code
   * auto nc = NetworkConfig::load(1, "3_parties.txt");
   * assert(nc.id() == 1);
   * assert(nc.networkSize() == 3);
   *
   * auto me = nc.party(1);
   * assert(me.hostname == "192.0.2.2");
   * assert(me.port == 5000);
   * @endcode
   *
   * Minor validation is performed on the file.
   *
   * @throws std::invalid_argument if \p my_id is too large.
   * @throws std::invalid_argument if \p filename contains an invalid row.
   * @throws std::invalid_argument if \p filename could not be opened.
   */
  static NetworkConfig load(std::size_t my_id, const std::string& filename);

  /**
   * @brief Create a network config where all parties are running locally.
   *
   * Creates a network config of a specified size, where all parties have
   * addresses on this machine.
   *
   * @code
   * auto nc = NetworkConfig::localhost(3, 5, 5000);
   *
   * for (int i = 0; i < nc.networkSize(); i++) {
   *   assert(nc.party(i).host == "127.0.0.1");
   *   assert(nc.party(i).port == 5000 + i);
   * }
   * @endcode
   *
   * @throws std::invalid_argument if <code>my_id >= size</code>.
   */
  static NetworkConfig localhost(std::size_t my_id,
                                 std::size_t size,
                                 std::size_t port_base);

  /**
   * @brief Create a network config where all parties are running locally.
   *
   * Like \ref NetworkConfig::localhost with \ref DEFAULT_LOCALHOST_PORT_OFFSET
   * as \p port_base.
   */
  static NetworkConfig localhost(std::size_t my_id, std::size_t size) {
    return NetworkConfig::localhost(my_id, size, DEFAULT_LOCALHOST_PORT_OFFSET);
  };

  /**
   * @brief Create a config from a list of parties.
   */
  NetworkConfig(std::size_t id, const std::vector<ConnectionInfo>& parties)
      : m_id(id), m_parties(parties) {
    validate();
  }

  /**
   * @brief Gets the identity of this party.
   */
  std::size_t id() const {
    return m_id;
  };

  /**
   * @brief Gets the size of the network.
   */
  std::size_t networkSize() const {
    return m_parties.size();
  };

  /**
   * @brief Get a list of connection information for parties in this network.
   */
  std::vector<ConnectionInfo> parties() const {
    return m_parties;
  };

  /**
   * @brief Get information about a party.
   */
  ConnectionInfo party(unsigned id) const {
    return m_parties[id];
  };

 private:
  std::size_t m_id;
  std::vector<ConnectionInfo> m_parties;

  void validate();
};

}  // namespace scl

#endif  // SCL_NET_CONFIG_H
