# Examples

This directory contains two examples of how to use SCL.

* `01_primitives.cc` shows how SCL works with primitives.
  
* `02_finite_fields.cc` shows how to work with finite fields in SCL.
  
* `03_secret_sharing.cc` shows how to work with both Shamir and additive secret
  sharing.
  
* `04_networking.cc` shows a how parties can be discovered in a dynamic fashion.

## Running the examples

Build and install SCL as instructed on the front page.

```
cmake . -B build
cmake --build build
```

which will build an executable for each example.
