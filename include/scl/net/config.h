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

#ifndef SCL_NET_CONFIG_H
#define SCL_NET_CONFIG_H

#include <cstddef>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

/**
 * @brief Default port offset used when all parties are running locally.
 */
#ifndef DEFAULT_PORT_OFFSET
#define DEFAULT_PORT_OFFSET 9900
#endif

namespace scl::net {

/**
 * @brief Connection information for a party.
 */
struct Party {
  /**
   * @brief The id of this party.
   */
  std::size_t id;

  /**
   * @brief The hostname.
   */
  std::string hostname;

  /**
   * @brief The base port.
   */
  std::size_t port;
};

/**
 * @brief Network configuration.
 *
 * A NetworkConfig is needed whenever by network objects in order to establish
 * connections to other nodes.
 */
class NetworkConfig {
 public:
  /**
   * @brief Load a network config from a file.
   * @param id the identity of this party
   * @param filename the filename
   */
  static NetworkConfig Load(std::size_t id, const std::string& filename);

  /**
   * @brief Create a network config where all parties are running locally.
   *
   * Because different processes cannot reuse the same port, the port argument
   * denotes a base from which a party's actual port is computed. Specifically,
   * party <code>i</code> will listen on <code>port_base + i</code> and connect
   * (as a client) to party <code>j</code> on <code>port_base + j</code>.
   *
   * @param id the identity of this party
   * @param size the size of the network
   * @param port_base the base port
   */
  static NetworkConfig Localhost(std::size_t id,
                                 std::size_t size,
                                 std::size_t port_base);

  /**
   * @brief Create a network config where all parties are running locally.
   * @param id the identity of this party
   * @param size the size of the network
   */
  static NetworkConfig Localhost(std::size_t id, std::size_t size) {
    return NetworkConfig::Localhost(id, size, DEFAULT_PORT_OFFSET);
  };

  /**
   * @brief Create a config from a list of parties.
   * @param id the id of the local party
   * @param parties a list of parties
   */
  NetworkConfig(std::size_t id, const std::vector<Party>& parties)
      : m_id(id), m_parties(parties) {
    Validate();
  };

  /**
   * @brief Create a network config for only one party.
   */
  NetworkConfig()
      : m_id(0), m_parties(std::vector<Party>{Party{0, "0.0.0.0", 0}}){};

  /**
   * @brief Gets the identity of this party.
   */
  std::size_t Id() const {
    return m_id;
  };

  /**
   * @brief Gets the size of the network.
   */
  std::size_t NetworkSize() const {
    return m_parties.size();
  };

  /**
   * @brief Get a list of connection information for parties in this network.
   */
  std::vector<Party> Parties() const {
    return m_parties;
  };

  /**
   * @brief Get information about a party.
   */
  Party GetParty(unsigned id) const {
    return m_parties[id];
  };

  /**
   * @brief Return a string representation of this network config.
   */
  std::string ToString() const;

 private:
  void Validate();

  std::size_t m_id;
  std::vector<Party> m_parties;
};

}  // namespace scl::net

#endif  // SCL_NET_CONFIG_H
