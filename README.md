# SCL — Secure Computation Library

SCL is a utilities library for prototyping Secure Multiparty Computation (MPC
for short) protocols. The focus of SCL is usability, both in terms of the
interfaces provided, but also the build process (SCL has no external
dependencies, for example). SCL moreover attempts to provide functionality that
abstracts away all the annoying "boilerplate" code that is needed for
implementing a new and exciting MPC protocol, such as implementing a finite
field, getting networking to work, or instantiating a PRG or hash function.

Hopefully, by using SCL, researches (and hobbyists) will find it a lot easier,
and quicker!, to implement MPC protocols.

### Disclaimer

SCL is distributed under the GNU Affero General Public License, for details,
refer to `LICENSE` or [https://www.gnu.org/licenses/](https://www.gnu.org/licenses/).

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.


# Building SCL

SCL has no external dependencies, except if you want to build the unittests. In
that case, [catch2](https://github.com/catchorg/Catch2/tree/v2.x) is required as
that's the framework used for testing, as well as ~lcov~ for generating test
coverage.

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

# Using SCL

To use SCL, link `libscl.so` when building your program and include the
`include/` directory to your builds includes. The "examples" directory has some
simple examples that use scl, as well as a CMake file that can be used as
inspiration.

# Documentation

SCL uses Doxygen for documentation. Run `./scripts/build_documentation.sh` to
generate the documentation. This is placed in the `doc/` folder. Documentation
uses `doxygen`, so make sure that's installed.
