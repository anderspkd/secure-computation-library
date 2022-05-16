/**
 * @file config.h
 *
 * SCL --- Secure Computation Library
 * Copyright (C) 2022 Anders Dalskov
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

#ifndef _SCL_NET_CONFIG_H
#define _SCL_NET_CONFIG_H

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

namespace scl {

/**
 * @brief Connection information for a party.
 */
struct Party {
  /**
   * @brief The id of this party.
   */
  unsigned id;

  /**
   * @brief The hostname.
   */
  std::string hostname;

  /**
   * @brief The base port.
   */
  int port;
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
  static NetworkConfig Load(unsigned id, std::string filename);

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
  static NetworkConfig Localhost(unsigned id, std::size_t size, int port_base);

  /**
   * @brief Create a network config where all parties are running locally.
   * @param id the identity of this party
   * @param size the size of the network
   */
  static NetworkConfig Localhost(unsigned id, std::size_t size) {
    return NetworkConfig::Localhost(id, size, DEFAULT_PORT_OFFSET);
  };

  /**
   * @brief Create a config from a list of parties.
   * @param id the id of the local party
   * @param parties a list of parties
   */
  NetworkConfig(unsigned id, std::vector<Party> parties)
      : mId(id), mParties(parties) {
    Validate();
  };

  /**
   * @brief Create a network config for only one party.
   */
  NetworkConfig()
      : mId(0), mParties(std::vector<Party>{Party{0, "0.0.0.0", 0}}){};

  /**
   * @brief Gets the identity of this party.
   */
  unsigned Id() const { return mId; };

  /**
   * @brief Gets the size of the network.
   */
  std::size_t NetworkSize() const { return mParties.size(); };

  /**
   * @brief Get a list of connection information for parties in this network.
   */
  std::vector<Party> Parties() const { return mParties; };

  /**
   * @brief Get information about a party.
   */
  Party GetParty(unsigned id) const { return mParties[id]; };

  /**
   * @brief Return a string representation of this network config.
   */
  std::string ToString() const;

 private:
  void Validate();

  unsigned mId;
  std::vector<Party> mParties;
};

}  // namespace scl

#endif /* _SCL_NET_CONFIG_H */
