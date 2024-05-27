# Introduction

**SCL** (short for *Secure Computation Library*) is a C++-20 library which aims
at removing a lot of the typical boilerplate code that one usually has to write
when developing a proof-of-concept for a new Secure Multiparty Computation (or
*MPC* for short) protocol.

Everything SCL is placed in the \ref scl namespace, and different
functionalities are placed in different sub namespaces, a short description of
each as well as their purpose is given here:

- \ref scl::coro coroutines.
- \ref scl::math math related stuff.
- \ref scl::net networking.
- \ref scl::proto protocol interfaces.
- \ref scl::sim protocol execution simulation.
- \ref scl::seri serialization.
- \ref scl::ss secret-sharing.
- \ref scl::util other utilities.
