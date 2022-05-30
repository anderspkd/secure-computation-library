/**
 * @file scl.h
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

#ifndef _SCL_SCL_H
#define _SCL_SCL_H

#include "scl/math.h"
#include "scl/networking.h"
#include "scl/primitives.h"
#include "scl/secret_sharing.h"

/**
 * @mainpage
 *
 * <p>SCL is a collection of C++ APIs that should aid a programmer in writing
 * Secure Multiparty Party, or <i>MPC</i>, protocol proof-of-concepts. Emphasis
 * has been placed on usability and readability, and so less attention have been
 * paid to making SCL extremely efficient.</p>
 *
 * <p>SCL provides APIs for several aspects of MPC protocols which are needed by
 * all implementations, but which are not typically "important" in the sense
 * that they are considered "implementation" details.</p>
 *
 * <h2>Math</h2>
 * Math related functionality is located in the headers in
 * <code>scl/math/</code>, or alternatively <code>scl/math.h</code>.
 *
 * <h2>Secret Sharing</h2>
 * SCL can perform both additive and Shamir secret-sharing. Include some of the
 * headers in <code>scl/ss/</code> or the <<code>scl/secret_sharing.h</code>
 * header for access.
 *
 * <h2>Networking</h2>
 * SCL provides some limited functionality when it comes to operating with a
 * network and peer-to-peer channels. This functionality can be accessed through
 * <code>scl/networking.h</code> or headers in <code>scl/net/</code>.
 */

/**
 * @brief Top level namespace.
 *
 * The main parts of SCL are placed in this namespace. Generally speaking,
 * functionality in this namespace will be well documented, thoroughly tested
 * and (where necessary and meaningful) will perform some form of input
 * validation.
 */
namespace scl {

/**
 * @brief Internal details, low level functions and other fun stuff.
 *
 * The <code>details</code> namespace contains internal details and helper
 * functions. The programmer cannot assume that all functionality in the
 * <code>details</code> namespace will perform input validation or have sane
 * behavior in case pre conditions are violated.
 */
namespace details {}
}  // namespace scl

#endif  // _SCL_SCL_H
