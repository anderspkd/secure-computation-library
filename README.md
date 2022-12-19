# SCL — Secure Computation Library

SCL is a utilities library for prototyping Secure Multiparty Computation (_MPC_
for short) protocols. The focus of SCL is usability, not necessarily speed. What
this means is that SCL strives to provide an intuitive, easy to use and
understand and well documented interface that helps the programmer prototype an
MPC protocol faster (and nicer) than if they had to write everything themselves.

SCL provides high level interfaces and functionality for working with
* Secret sharing, additive and Shamir.
* Finite fields.
* Networking.
* Primitives, such as hash functions and PRGs.

### Disclaimer

SCL is distributed under the GNU Affero General Public License, for details,
refer to `LICENSE` or [https://www.gnu.org/licenses/](https://www.gnu.org/licenses/).

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.

# Building SCL

SCL uses [gmp](https://gmplib.org/) for working with Elliptic Curves, and
[catch2](https://github.com/catchorg/Catch2/tree/v2.x) for testing and `lcov`
for test coverage.

The CMake file recongnizes two different build types: `Debug` and `Release`, the
latter being the default. In either case, building is straight forward and can
be done with the commands

```
cmake . -B build -DCMAKE_BUILD_TYPE=<Debug|Release>
cmake --build build
```

In case the `Release` build is used, SCL can be installed by running

```
sudo cmake --install build
```

after the build command. By default, headers are install in `usr/local/include`
and the shared library in `/usr/local/lib`. This location can be controlled by
setting the `CMAKE_INSTALL_PREFIX` accordingly.

Support for Elliptic Curves can be disabled (and thus remove the need to have
gmp installed) by passing `-DWITH_EC=OFF` to cmake.


# Using SCL

To use SCL, link `libscl.so` when building your program and include the
`include/` directory to your builds includes. The "examples" directory has some
simple examples that use scl, as well as a CMake file that can be used as
inspiration.

# Documentation

SCL uses Doxygen for documentation. Run `./scripts/build_documentation.sh` to
generate the documentation. This is placed in the `doc/` folder. Documentation
uses `doxygen`, so make sure that's installed.

# Citing

I'd greatly appreciate any work that uses SCL include the below bibtex entry

```
@misc{secure-computation-library,
    author = {Anders Dalskov},        
    title = {{SCL (Secure Computation Library)---utility library for prototyping MPC applications}},
    howpublished = {\url{https://github.com/anderspkd/secure-computation-library}},
}
```
