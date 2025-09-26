#ifndef SCL_NET_TCP_NETWORK_H
#define SCL_NET_TCP_NETWORK_H

#include "scl/coro/task.h"
#include "scl/net/config.h"
#include "scl/net/network.h"

namespace scl {

/**
 * @brief Create a network using a network config.
 * @param config the network configuration to use.
 *
 * Creates a new network where the connection information about the parties of
 * the network is read from a provided config. In the resulting network, the
 * local party is connected to itself with a LoopbackChannel, and to everyone
 * else with a TcpChannel.
 */
Task<Network> createTcpNetwork(const NetworkConfig& config);

}  // namespace scl

#endif  // SCL_NET_TCP_NETWORK_H
